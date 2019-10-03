/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include <libsbox/daemon.h>
#include <libsbox/utils.h>

#include <cstring>
#include <fcntl.h>
#include <syslog.h>
#include <unistd.h>

/*
 * libsboxd is systemd service, which maintain a set of containers. It can be used through unix-socket by sending JSON
 * request and waiting for response. Unix-socket path may be changed in config.
 * Protocol:
 * 1. Connect to socket
 * 2. Write JSON request to socket
 * 3. Wait for JSON response
 * 4. Close connection
 *
 * Request example in request.json, response example in response.json
 *
 * IMPORTANT: what is said in the next paragraph is not yet implemented TODO
 * There are two types of errors: evaluation errors and internal. While evaluations errors are reported just by
 * returning json object with only one field "error" (e.g. {"error": "Executable not found"}), second are critical and
 * lead to libsboxd termination.
 *
 * libsboxd uses following processes tree:
 * daemon
 * ├── proxy
 * │   ├── worker
 * │   │   └── target
 * |   ├── worker
 * │   │   └── target
 * │   ...
 * ├── proxy
 * │   ├── worker
 * │   │   └── target
 * |   ├── worker
 * │   │   └── target
 * │   ...
 * ...
 *
 * Daemon process is systemd service, which creates unix-socket and spawn fixed amount of proxy processes (number may be
 * changed in config).
 * Proxy process accepts connections on unix-socket, receives query and after processing sends response and closes
 * connection. It creates pipes and prepares workers' structures, which lie in memory, shared between proxy and workers.
 * Proxy and worker processes share file descriptor table, so pipes, created in proxy process are also visible in worker
 * processes.
 * Worker process run in namespaces, so if any error occurs, to cleanup worker need to just exit and namespaces
 * will do the rest.
 *
 * Synchronization protocol of proxy and worker process:
 * proxy:  get request -> open pipes -> fill desc(s) -> [     sub 1     ] -----------------------------> [start barrier(k)] -> close pipes -----> [end barrier(k)]
 * worker: -------------------------------------------> [desc barrier(1)] -> prepare and spawn target -> [     sub 1      ] -> wait for target -> [     sub 1    ]
 * First barrier stands for waiting proxy to fill target_desc. After second barrier all targets are spawned, so we can
 * close pipes in proxy and workers. And after third execution is completed and we call collect results from target_desc.
 *
 * Target process in spawned by worker and after some preparations executes target executable.
 * TODO: https://www.freedesktop.org/software/systemd/man/daemon.html#New-Style%20Daemons
 * TODO: remove syslog
 * TODO: non-critical request error handling
 * TODO: service stop
 */

void err(const std::string &msg) {
    syslog(LOG_ERR, "%s", msg.c_str());
    exit(1);
}

#include <libsbox/target_desc.h>
#include <iostream>

int main(int argc, char *argv[]) {
    if (argc == 2 && strcmp(argv[1], "stop") == 0) {
        exit(system("(cat /run/libsboxd.pid | xargs kill -s SIGTERM); rm /run/libsboxd.pid"));
    }

    openlog("libsboxd", LOG_NDELAY | LOG_PERROR, LOG_DAEMON);
    int fd = open("/run/libsboxd.pid", O_CREAT | O_EXCL | O_WRONLY);
    if (fd < 0) {
        err(format("Cannot create /run/libsboxd.pid: %m"));
    }
    if (dprintf(fd, "%d", getpid()) < 0) {
        err(format("Failed to write pid: %m"));
    }
    if (close(fd) != 0) {
        err(format("Failed to close pid file: %m"));
    }

    Daemon::get().run();
}
