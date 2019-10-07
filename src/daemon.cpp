/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include <libsbox/daemon.h>
#include <libsbox/utils.h>
#include <libsbox/signals.h>
#include <libsbox/config.h>
#include <libsbox/worker.h>

#include <unistd.h>
#include <filesystem>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <syslog.h>
#include <signal.h>
#include <wait.h>

namespace fs = std::filesystem;

Daemon Daemon::daemon_;

Daemon &Daemon::get() {
    return daemon_;
}

[[noreturn]]
void Daemon::die(const std::string &error) {
    syslog(LOG_ERR, "%s", ("[daemon] " + error).c_str());
    unlink(socket_path_.c_str());
    close(server_socket_fd_);
    unlink("/run/libsboxd.pid");
    // Kill every process in group, to ensure that no process continue running uncontrolled
    kill(0, SIGKILL);
    exit(1);
}

void Daemon::terminate() {
    terminated_ = true;
}

[[noreturn]]
void Daemon::run() {
    ContextManager::set(this);
    prepare();

    // Spawn workers
    num_boxes_ = Config::get().get_num_boxes();
    workers_pids_.reserve(num_boxes_);
    for (int i = 0; i < num_boxes_; ++i) {
        pid_t pid = Worker::spawn();
        if (pid == 0) {
            die(format("Failed to spawn worker number %d: %m", i + 1));
        }
        workers_pids_.push_back(pid);
    }

    while (true) {
        // Wait for signal or for any worker to exit
        int status;
        pid_t pid = wait(&status);
        if (pid < 0) {
            if (!terminated_) {
                die(format("wait() failed: %m"));
            }
            break;
        } else {
            // We must die here, because worker shall not exit itself without command from daemon
            if (WIFEXITED(status)) {
                die(format("Process %d exited with exitcode %d", pid + 1, WEXITSTATUS(status)));
            } else {
                die(format("Process %d was signaled with signal %s", pid + 1, strsignal(WTERMSIG(status))));
            }
        }
    }

    if (unlink(socket_path_.c_str()) != 0) {
        die(format("Failed to unlink() socket: %m"));
    }
    if (close(server_socket_fd_) != 0) {
        die(format("Failed to close() socket: %m"));
    }

    // TODO: better termination
    if (kill(0, SIGKILL) != 0) {
        die(format("kill() failed: %m"));
    }

//    while (true) {
//        int status;
//        pid_t pid = wait(&status);
//        if (pid < 0) {
//            if (errno == ECHILD) {
//                break;
//            }
//            die(format("wait() failed: %m"));
//        }
//    }

    exit(0);
}

void Daemon::prepare() {
    // We want libsbox to be run as root
    if (setresuid(0, 0, 0) != 0) {
        die(format("Cannot change to root user: %m"));
    }
    if (setresgid(0, 0, 0) != 0) {
        die(format("Cannot change to root group: %m"));
    }

    umask(022);

    // Although we don't use exceptions, some parts of stl as well as nlohmann/json do. So if exception occurs and is
    // not catched we want die gracefully
    std::set_terminate([]() {
        ContextManager::get().die("Uncaught exception");
    });

    prepare_signals();

    socket_path_ = Config::get().get_socket_path();

    // Remove socket if exists
    unlink(socket_path_.c_str());
    errno = 0;

    // Create UNIX socket, on which libsboxd will serve
    server_socket_fd_ = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_socket_fd_ < 0) {
        die(format("Failed to create UNIX socket: %m"));
    }

    sockaddr_un addr{};
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path_.c_str(), sizeof(addr.sun_path) - 1);
    if (bind(server_socket_fd_, (const sockaddr *) &addr, sizeof(sockaddr_un)) != 0) {
        die(format("Failed to bind UNIX socket to @%s: %m", socket_path_.c_str()));
    }

    if (listen(server_socket_fd_, 0) != 0) {
        die(format("Failed to start listening on socket: %m"));
    }

    // We move daemon process to new process group to easily kill all forked processes
    if (setpgrp() != 0) {
        die(format("Failed to move process to new process group: %m"));
    }

    // All containers must have distinct user ids, so we will use this shared counter to obtain ids in
    uid_counter_ = new SharedCounter(Config::get().get_first_uid());
}

int Daemon::get_server_socket_fd() const {
    return server_socket_fd_;
}

SharedCounter *Daemon::get_uid_counter() const {
    return uid_counter_;
}
