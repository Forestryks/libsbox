/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include <libsbox/daemon.h>
#include <libsbox/config.h>
#include <libsbox/utils.h>
#include <libsbox/proxy.h>
#include <libsbox/context.h>
#include <libsbox/signals.h>

#include <cstdlib>
#include <syslog.h>
#include <unistd.h>
#include <filesystem>
#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <csignal>
#include <wait.h>
#include <sys/stat.h>
#include <algorithm>
#include <fcntl.h>

namespace fs = std::filesystem;

Daemon Daemon::daemon_;

Daemon &Daemon::get() {
    return daemon_;
}

[[noreturn]]
void Daemon::run() {
    prepare();

    // Spawn proxies
    num_boxes_ = Config::get().get_num_boxes();
    proxy_pids_.reserve(num_boxes_);
    for (int i = 0; i < num_boxes_; ++i) {
        pid_t pid = Proxy::spawn();
        if (pid == 0) {
            die(format("Failed to spawn proxy number %d: %m", i + 1));
        }
        proxy_pids_.push_back(pid);
    }

    while (true) {
        // Wait for signal or for any proxy to exit
        int status;
        pid_t pid = wait(&status);
        if (pid < 0) {
            if (errno != EINTR) {
                die(format("wait() failed: %m"));
            }
            if (interrupt_signal != SIGTERM) {
                die(format("wait() interrupted with %s", strsignal(interrupt_signal)));
            }
            break;
        } else {
            int id = std::find(proxy_pids_.begin(), proxy_pids_.end(), pid) - proxy_pids_.begin();
            if (id == (int) proxy_pids_.size()) {
                die(format("Unknown process exited (%d)", pid));
            }
            // We must die here, because proxy shall not exit itself without command from daemon
            if (WIFEXITED(status)) {
                die(format("Proxy %d exited with exitcode %d", pid + 1, WEXITSTATUS(status)));
            } else {
                die(format("Proxy %d was signaled with signal %s", pid + 1, strsignal(WTERMSIG(status))));
            }
        }
    }

    for (int i = 0; i < (int) proxy_pids_.size(); ++i) {
        if (kill(proxy_pids_[i], SIGTERM) != 0) {
            die(format("Failed to kill proxy %d: %m", i + 1));
        }
    }

    for (int i = 0; i < (int) proxy_pids_.size(); ++i) {
        int status;
        pid_t pid = wait(&status);
        if (pid < 0) {
            die(format("wait() failed: %m"));
        } else {
            auto iter = std::find(proxy_pids_.begin(), proxy_pids_.end(), pid);
            if (iter == proxy_pids_.end()) {
                die(format("Unknown process exited (%d)", pid));
            }
            proxy_pids_.erase(iter);
        }
    }

    cleanup();
    exit(0);
}

void Daemon::prepare() {
    Context::set_context(this);

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
        Context::get().die("Uncaught exception");
    });

    prepare_signals();

    socket_path_ = Config::get().get_socket_path();

    // Remove socket file if it exists
    unlink(socket_path_.c_str());

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
}

void Daemon::cleanup() {
    unlink(socket_path_.c_str());
    close(server_socket_fd_);
}

[[noreturn]]
void Daemon::die(const std::string &error) {
    syslog(LOG_ERR, "%s", ("[daemon] " + error).c_str());
    unlink(socket_path_.c_str());
    close(server_socket_fd_);
    // Kill every process in group, to ensure that no process continue running uncontrolled
    kill(0, SIGKILL);
    exit(1);
}

[[nodiscard]]
int Daemon::get_server_socket_fd() const {
    return server_socket_fd_;
}
