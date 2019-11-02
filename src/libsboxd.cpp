/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

/*
 * libsboxd is fast invoker for contest management systems (CMS). It runs in background as systemd service and uses
 * UNIX-socket for queries.
 *
 * Running contestants' submissions is a complex task. Invokers are programs designed to do this.
 * These are requirements for invoker:
 * 1. Sandboxing. Submitted code is untrusted and we don't know what will it do besides solving the task. Contestant may
 * intentionally try to break CMS (which usually is considered as rules violation) or occasionally make something after
 * which it can no longer function properly. Another problem is that solution can try accessing something which is not
 * intended for it (such as tests' answers). So we need sandboxing to prevent such scenarios.
 * 2. Time and memory limits. We need ensure that submission not consume more time and memory than intended in certain
 * problem.
 * 3. Resource usage measurement and exit status. To properly set submission's verdict on test, we must know how it
 * terminated and it's resource usage (time and memory usage).
 * 4. Errors-safety. If an error occurs in any part of invoker, it must ensure that nothing keeps running uncontrolled.
 * 5. (optional) Multiple processes running simultaneously and communication between them (e.g. in interactive problems)
 *
 * libsboxd uses the following to satisfy requirements:
 * 1. Linux namespaces provide nice mechanism to isolate certain resources from outer system.
 * 2. Different mounts are used to create sandbox directory setup. Bind mount perfectly work through chroot jail and
 * provide additional permission options.
 * 3. Cgroups are used for time and memory limits and measurement. This mechanism work great with multiple processes in
 * a single box.
 * 4. We also use rlimits to limit some of resources.
 * 5. Chroot jail is of course used too.
 * 6. Pipes are used for redirecting streams between different processes running simultaneously.
 *
 * libsboxd maintain the following process tree:
 * daemon
 * ├── worker
 * │   ├── container
 * │   │   └── slave
 * |   ├── container
 * │   │   └── slave
 * │   ...
 * ├── worker
 * │   ├── container
 * │   │   └── slave
 * |   ├── container
 * │   │   └── slave
 * │   ...
 * ...
 *
 * Daemon process is systemd service itself, which creates unix-socket and spawn certain amount of worker processes
 * (number may be changed in config).
 * Worker process accepts connections on unix-socket, receives query and after processing sends response and closes
 * connection. It creates pipes and prepares containers' structures, which lie in memory, shared between worker and
 * containers. Worker and container processes share file descriptor table, so pipes, created in worker process are also
 * visible in container processes.
 * Container process run in namespaces, so if any error occurs, to cleanup container need to just exit and namespaces
 * will do the rest.
 * Slave process in spawned by container and after some preparations executes target executable.
 *
 * +--------------------------------------------------------------------------------------------------------------+
 * |                             Timeline of query processing (columns are processes)                             |
 * +-----------------------------------+-----------------------------------+--------------------------------------+
 * |               Worker              |             Containers            |                Slaves                |
 * +-----------------------------------+-----------------------------------+--------------------------------------+
 * | Get request                       |                                   |                                      |
 * +-----------------------------------+-----------------------------------+                                      |
 * | Ensure that we have enough        | Actions on container creation:    |                                      |
 * | containers. If not, spawn as many |  - create container process in    |                                      |
 * | as necessary                      | new namespaces using clone()      |                                      |
 * |                                   |  - prepare working directory and  |                                      |
 * |                                   | create necessary mounts (working  |                                      |
 * |                                   | directory setup is shown below)   |                                      |
 * |                                   |  - disable ipcs (if needed)       |                                      |
 * +-----------------------------------+-----------------------------------+                                      |
 * |  - Create pipes                   |                                   |                                      |
 * |  - Write tasks data to containers |                                   |                                      |
 * +-----------------------------------+-----------------------------------+                                      |
 * |      [synchronized] Container waits for worker to write task data     |                                      |
 * +-----------------------------------+-----------------------------------+                                      |
 * |                                   |  - Create run-specific mounts     |                                      |
 * |                                   |  - Create cgroups                 |                                      |
 * |                                   +-----------------------------------+--------------------------------------+
 * |                                   | fork() into slave                 | Actions on slave process creation:   |
 * |                                   |                                   |  - open target executable            |
 * |                                   |                                   |  - chdir()                           |
 * |                                   |                                   |                                      |
 * |                                   |                                   |  - prepare file descriptors          |
 * |                                   |                                   |  - enter cgroups                     |
 * |                                   |                                   |  - setup rlimits                     |
 * |                                   |                                   |  - chroot()                          |
 * |                                   |                                   |  - drop privileges                   |
 * |                                   |                                   |  - exec()                            |
 * +-----------------------------------+-----------------------------------+--------------------------------------+
 * |     [synchronized] Worker waits for ALL containers to start slave     |                                      |
 * +-----------------------------------+-----------------------------------+                                      |
 * | Close pipes                       | Wait for slave to exit and        |                                      |
 * |                                   | collect results                   |                                      |
 * +-----------------------------------+-----------------------------------+                                      |
 * |                                   |  - Destroy run-specific mount     |                                      |
 * |                                   |  - Destroy cgroups                |                                      |
 * +-----------------------------------+-----------------------------------+                                      |
 * |   [synchronized] Worker waits for ALL containers to collect results   |                                      |
 * +-----------------------------------+-----------------------------------+                                      |
 * | Send response                     |                                   |                                      |
 * +-----------------------------------+-----------------------------------+--------------------------------------+
 *
 * When using libsboxd you must follow this protocol:
 * 1. Connect to socket
 * 2. Write JSON request to socket
 * 3. Wait for JSON response
 * 4. Close connection
 *
 * You can find request example in request.json and response example in response.json
 *
 * IMPORTANT: what is said in the next paragraph is not yet implemented. Currently all errors lead to libsboxd shutdown TODO
 * There are two types of errors: evaluation errors and internal. While evaluations errors are reported just by
 * returning json object with only one field "error" (e.g. {"error": "Executable not found"}), second are critical and
 * lead to libsboxd termination.
 *
 * TODO: https://www.freedesktop.org/software/systemd/man/daemon.html#New-Style%20Daemons
 * TODO: remove syslog
 * TODO: non-critical request error handling
 * TODO: service stop
 */

#include "daemon.h"

#include <cstdlib>
#include <cstring>

int main(int argc, char *argv[]) {
    if (argc == 2 && strcmp(argv[1], "stop") == 0) {
        exit(system("(cat /run/libsboxd.pid | xargs kill -s SIGTERM); rm /run/libsboxd.pid"));
    }
    if (argc == 2 && strcmp(argv[1], "kill") == 0) {
        exit(system("(cat /run/libsboxd.pid | xargs kill -s SIGKILL); rm /run/libsboxd.pid"));
    }
    if (argc == 2 && strcmp(argv[1], "killall") == 0) {
        exit(system("rm /run/libsboxd.pid; killall libsboxd -s SIGKILL"));
    }

    Daemon::get().run();
}
