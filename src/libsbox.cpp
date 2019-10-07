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
 * ├── worker
 * │   ├── container
 * │   │   └── task
 * |   ├── container
 * │   │   └── task
 * │   ...
 * ├── worker
 * │   ├── container
 * │   │   └── task
 * |   ├── container
 * │   │   └── task
 * │   ...
 * ...
 *
 * Daemon process is systemd service, which creates unix-socket and spawn fixed amount of worker processes (number may be
 * changed in config).
 * Worker process accepts connections on unix-socket, receives query and after processing sends response and closes
 * connection. It creates pipes and prepares containers' structures, which lie in memory, shared between worker and containers.
 * Worker and container processes share file descriptor table, so pipes, created in worker process are also visible in container
 * processes.
 * Container process run in namespaces, so if any error occurs, to cleanup container need to just exit and namespaces
 * will do the rest.
 *
 * Synchronization protocol of worker and container process:
 * worker:    get request -> open pipes -> fill desc(s) -> [     sub 1     ] ----------------------------> [start barrier(k)] -> close pipes ---------------> [end barrier(k)]
 * container: -------------------------------------------> [desc barrier(1)] -> prepare and start tasks -> [     sub 1      ] -> wait for task to complete -> [     sub 1    ]
 * First barrier stands for waiting worker to fill task_data. After second barrier all task are started, so we can
 * close pipes in worker and containers. And after third execution is completed and we call collect results from task_data.
 *
 * Task process in spawned by container and after some preparations executes task target executable.
 * TODO: https://www.freedesktop.org/software/systemd/man/daemon.html#New-Style%20Daemons
 * TODO: remove syslog
 * TODO: non-critical request error handling
 * TODO: service stop
 */

void err(const std::string &msg) {
    syslog(LOG_ERR, "%s", msg.c_str());
    exit(1);
}

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
