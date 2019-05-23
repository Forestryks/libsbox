/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include <libsbox/execution_context.h>
#include <libsbox/signal.h>
#include <libsbox/die.h>
#include <libsbox/conf.h>
#include <libsbox/fs.h>

#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <signal.h>
#include <wait.h>

void libsbox::execution_context::register_target(libsbox::execution_target *target) {
    this->targets.push_back(target);
}

void libsbox::execution_context::link(libsbox::out_stream *write_end, libsbox::in_stream *read_end, int pipe_flags) {
    this->pipes.push_back({write_end, read_end, pipe_flags});
}

void libsbox::execution_context::create_pipes() {
    for (const auto &pipe : this->pipes) {
        int fd[2];
        if (pipe2(fd, pipe.extra_flags) != 0) {
            libsbox::die("Cannot create pipe: pipe2 failed (%s)", strerror(errno));
        }
        if (pipe.write_end->fd != -1 || !pipe.write_end->filename.empty()) {
            libsbox::die("Cannot create pipe: write end busy");
        }
        if (pipe.read_end->fd != -1 || !pipe.write_end->filename.empty()) {
            libsbox::die("Cannot create pipe: read end busy");
        }
        pipe.write_end->fd = fd[1];
        pipe.read_end->fd = fd[0];

        if (fcntl(fd[1], F_SETPIPE_SZ, pipe_size) < pipe_size) {
            libsbox::die("Cannot increase pipe capacity to %i bytes (%s)", pipe_size,
                         strerror(errno));
        }
    }

    if (pipe2(this->error_pipe, O_NONBLOCK | O_DIRECT) != 0) {
        libsbox::die("Cannot create error pipe: pipe2 failed (%s)", strerror(errno));
    }

    for (auto target : this->targets) {
        if (pipe2(target->status_pipe, 0) != 0) {
            libsbox::die("Cannot create status pipe: pipe2 failed (%s)", strerror(errno));
        }
    }
}

void libsbox::execution_context::destroy_pipes() {
    for (const auto &pipe : this->pipes) {
        if (close(pipe.write_end->fd) != 0) {
            libsbox::die("Cannot close write end of pipe: (%s)", strerror(errno));
        }
        if (close(pipe.read_end->fd) != 0) {
            libsbox::die("Cannot close read end of pipe (%s)", strerror(errno));
        }

        pipe.write_end->fd = -1;
        pipe.read_end->fd = -1;
    }

    if (close(this->error_pipe[0]) != 0) {
        libsbox::die("Cannot close read end of error pipe (%s)", strerror(errno));
    }

    for (auto target : this->targets) {
        if (close(target->status_pipe[0]) != 0) {
            libsbox::die("Cannot close read end of status pipe (%s)", strerror(errno));
        }
    }
}

#include <iostream>

void libsbox::execution_context::run() {
    current_context = this;

    for (auto target : this->targets) {
        target->prepare();
    }

    this->create_pipes();

    for (auto target : this->targets) {
        target->start_proxy();
        std::cout << target->proxy_pid << std::endl;
    }

    if (close(this->error_pipe[1]) != 0) {
        libsbox::die("Cannot close write end of error pipe (%s)", strerror(errno));
    }

    int status;
    wait(&status);

    char buf[err_buf_size];
    if (read(this->error_pipe[0], buf, err_buf_size) != 0) {
        libsbox::die("%s", buf);
    }
    // wait for process

//    sleep(1000);

    this->destroy_pipes();

    for (auto target : this->targets) {
        target->cleanup();
    }

    current_context = nullptr;
}

void libsbox::execution_context::die() {
    for (auto target : this->targets) {
        target->die();
    }
}

namespace libsbox {
    execution_context *current_context = nullptr;
} // namespace libsbox
