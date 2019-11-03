/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include "daemon.h"
#include "config.h"
#include "signals.h"

#include <iostream>
#include <wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <limits.h>

namespace fs = std::filesystem;

Daemon Daemon::daemon_;

Daemon &Daemon::get() {
    return daemon_;
}

void Daemon::die(const std::string &error) {
    log(error);
    unlink(socket_path_.c_str());
    close(server_socket_fd_);
    unlink("/run/libsboxd.pid");
    // Thanks to prctl(PR_SET_PDEATHSIG) we can just exit here, and child processes will exit themselves
    _exit(1);
}

void Daemon::terminate() {
    terminated_ = true;
}

void Daemon::run() {
    ContextManager::set(this);
    prepare();

    pipe2(error_pipe_, O_CLOEXEC | O_DIRECT | O_NONBLOCK);

    // Spawn workers
    num_boxes_ = Config::get().get_num_boxes();
    workers_.reserve(num_boxes_);
    for (int i = 0; i < num_boxes_; ++i) {
        Worker *worker = new Worker(server_socket_fd_, id_getter_.get());
        if (worker->start() < 0) {
            die(format("Failed to spawn worker: %m"));
        }
        workers_.emplace_back(worker);
    }

    if (close(error_pipe_[1]) != 0) {
        die(format("Cannot close write end of error pipe: %m"));
    }

    log("Started, waiting for connections");

    while (true) {
        // Wait for signal or for any worker to exit
        int status;
        pid_t pid = wait(&status);
        if (pid < 0) {
            if (!terminated_) {
                die(format("Failed to wait for any worker to exit: %m"));
            }
            break;
        } else {
            // We must die here, because worker shall not exit itself without command from daemon
            die_with_worker_status(status);
        }
    }

    if (unlink(socket_path_.c_str()) != 0) {
        die(format("Failed to unlink() socket: %m"));
    }
    if (close(server_socket_fd_) != 0) {
        die(format("Failed to close() socket: %m"));
    }

    // Kill all workers and after it wait until no workers continue running
    for (auto &worker : workers_) {
        if (kill(worker->get_pid(), SIGTERM) != 0) {
            die(format("Failed to send SIGTERM to worker: %m"));
        }
    }

    for (int i = 0; i < (int) workers_.size(); ++i) {
        int status;
        pid_t pid = wait(&status);
        if (pid < 0) {
            die(format("Failed to wait() for worker: %m"));
        }
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            die_with_worker_status(status);
        }
    }

    log("Stopped");

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

    int fd = open("/run/libsboxd.pid", O_CREAT | O_EXCL | O_WRONLY);
    if (fd < 0) {
        // We don't call die() here, because it will try to cleanup and remove /run/libsboxd.pid even if current process
        // don't own it
        log(format("Cannot create /run/libsboxd.pid: %m"));
        exit(1);
    }
    if (dprintf(fd, "%d", getpid()) < 0) {
        die(format("Failed to write pid to /run/libsboxd.pid: %m"));
    }
    if (close(fd) != 0) {
        die(format("Failed to close /run/libsboxd.pid: %m"));
    }

    std::set_terminate(
        []() {
            ContextManager::get().die("Uncaught exception");
        }
    );

    prepare_signals();

    socket_path_ = Config::get().get_socket_path();

    // Remove socket if exists
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

    // All containers must have distinct user ids, so we will use this shared getter to obtain ids
    id_getter_ = std::make_unique<SharedIdGetter>(Config::get().get_first_uid(), 256);
}

void Daemon::close_error_pipe_read_end() {
    if (close(error_pipe_[0]) != 0) {
        ContextManager::get().die("Cannot close read end of error pipe: %m");
    }
}

void Daemon::log(const std::string &error) {
    std::cerr << "[daemon] " << error << std::endl;
}

void Daemon::report_error(const std::string &error) {
    write(error_pipe_[1], error.c_str(), PIPE_BUF);
}

void Daemon::die_with_worker_status(int status) {
    char buf[PIPE_BUF + 1];
    int cnt = read(error_pipe_[0], buf, PIPE_BUF);
    if (cnt < 0 && errno != EAGAIN) {
        die(format("Cannot read from error pipe: %m"));
    }
    if (cnt > 0) {
        buf[cnt] = 0;
        die(format("Reported error: %s", buf));
    }

    if (WIFEXITED(status)) {
        die(format("Worker exited with exitcode %d", WEXITSTATUS(status)));
    } else {
        die(format("Worker was killed with signal %d (%s)", WTERMSIG(status), strsignal(WTERMSIG(status))));
    }
}

int Daemon::get_error_pipe_fd() {
    return error_pipe_[1];
}
