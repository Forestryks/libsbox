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
 */

// CLOSED(#0@forestryks): optimize includes
// CLOSED(#1@forestryks): remove debug output
// CLOSED(#2@forestryks): remove #include <iostream>
// CLOSED(#3@forestryks): don't close error pipe before exec
// CLOSED(#4@forestryks): -2 = dup stdout
// CLOSED(#5@forestryks): disable standard rules parameter
// WONTFIX(#6@forestryks): setters/getters in task_data upd:reopened
// WONTFIX(#7@forestryks): error pipe capacity?
// CLOSED(#8@forestryks): logging
// WONTFIX?(#9@forestryks): optimize memory (delete all unnecessary before entering cgroups)
// CLOSED(#10@forestryks): save in Bind
// CLOSED(#11@forestryks): check memory usage overhead
// WONTFIX?(#12@forestryks): non-critical request error handling
// CLOSED(#13@forestryks): https://www.freedesktop.org/software/systemd/man/daemon.html#New-Style%20Daemons
// CLOSED(#14@forestryks): cleanup cgroups
// WONTFIX(#15@forestryks): constructors and destructors must not create/destroy shared data
// CLOSED(#16@forestryks): errors in plain structures
// CLOSED(#17@forestryks): consider using exceptions
// CLOSED(#18@forestryks): use correct data types
// TODO(#19@forestryks): update description + add dir setup
// WONTFIX(#20@forestryks): ownership and reset in shared_barrier (may be fixed automatically after #15)
// WONTFIX(#21@forestryks): normal debug
// CLOSED(#22@forestryks): never use exit(), use _exit()
// CLOSED(#23@forestryks): restore default SIGCHLD in containers
// CLOSED(#24@forestryks): don't use C-style casts
// TODO(#25@forestryks): add tests (esp for pipes)
// CLOSED(#26@forestryks): always use die()
// CLOSED(#27@forestryks): synchronize in slave too
// CLOSED(#28@forestryks): don't update config on install
// CLOSED(#29@forestryks): race condition when opening/closing fds (upd: there is no race condition in fact)
// CLOSED(#30@forestryks): memory controller failcnt
// WONTFIX?(#31@forestryks): re-read memory controller to ensure that limit is set
// CLOSED(#32@forestryks): omm_killed and memory_limit_hit multi-threading support
// CLOSED(#33@forestryks): use STD*_FILENO
// WONTFIX?(#34@forestryks): write logs?
// CLOSED(#35@forestryks): kill_all() after first child exited
// CLOSED(#35@forestryks): correct data types in task_data
// CLOSED(#36@forestryks): rewrite libsbox.h
// CLOSED(#37@forestryks): move shared structures to libraries
// CLOSED(#38@forestryks): error handling in json (mb check schema first)
// CLOSED(#39@forestryks): ignore inputs/outputs (/dev/null)
// TODO(#40@forestryks): connection errors handling?
// CLOSED(#41@forestryks): write cgroups immediately before exec
// TODO(#42@forestryks): cleanup ipc?
// CLOSED(#43@forestryks): cleanup working directory?
// TODO(#44@forestryks): use group for file permissions?
// TODO(#45@forestryks): rewrite container?
// CLOSED(#46@forestryks): environment and PATH
// TODO(#47@forestryks): check credentials?
// TODO(#47@forestryks): environment

#include "daemon.h"

#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <memory>

int main(int argc, char *argv[]) {
    if (argc == 2 && strcmp(argv[1], "stop") == 0) {
        _exit(system("(cat /run/libsboxd.pid | xargs kill -s SIGTERM); rm /run/libsboxd.pid"));
    }
    if (argc == 2 && strcmp(argv[1], "kill") == 0) {
        _exit(system("(cat /run/libsboxd.pid | xargs kill -s SIGKILL); rm /run/libsboxd.pid"));
    }
    if (argc == 2 && strcmp(argv[1], "killall") == 0) {
        _exit(system("rm /run/libsboxd.pid; killall libsboxd -s SIGKILL"));
    }

    std::make_unique<Daemon>()->run();
}
