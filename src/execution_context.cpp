/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include <libsbox/execution_context.h>
#include <libsbox/signal.h>
#include <libsbox/die.h>
#include <libsbox/conf.h>

#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <signal.h>

void libsbox::execution_context::register_target(libsbox::execution_target *target) {
    this->targets.push_back(target);
}

void libsbox::execution_context::link(libsbox::out_stream *write_end, libsbox::in_stream *read_end, int pipe_flags) {
    this->pipes.push_back({write_end, read_end, pipe_flags});
}

void libsbox::execution_context::link(const std::string &filename, libsbox::in_stream *stream, int open_flags) {
    this->input_files.push_back({filename, stream, open_flags});
}

void libsbox::execution_context::link(libsbox::out_stream *stream, const std::string &filename, int open_flags) {
    this->output_files.push_back({stream, filename, open_flags});
}

void libsbox::execution_context::create_pipes() {
    for (const auto &pipe : this->pipes) {
        int fd[2];
        if (pipe2(fd, pipe.extra_flags) != 0) {
            libsbox::die("Cannot create pipe: pipe2 failed (%s)", strerror(errno));
        }
        if (pipe.write_end->fd != -1) {
            libsbox::die("Cannot create pipe: write end busy");
        }
        if (pipe.read_end->fd != -1) {
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

namespace libsbox {
    int clone_callback(void *);
} // namespace libsbox

int libsbox::clone_callback(void *arg) {
    return current_target->proxy();
}

void libsbox::execution_context::run() {
    current_context = this;

    for (auto target : this->targets) {
        target->prepare();
    }

    this->create_pipes();

    for (auto target : this->targets) {
        current_target = target;

        char *clone_stack = new char[clone_stack_size];
        target->proxy_pid = clone(
            clone_callback,
            clone_stack + clone_stack_size,
            SIGCHLD,
            nullptr
        );
        delete[] clone_stack;

        current_target = nullptr;
        if (target->proxy_pid < 0) {
            libsbox::die("Cannot create proxy: fork failed (%s)", strerror(errno));
        }

        if (close(target->status_pipe[1]) != 0) {
            libsbox::die("Cannot close write end of status pipe (%s)", strerror(errno));
        }
    }

    if (close(this->error_pipe[1]) != 0) {
        libsbox::die("Cannot close write end of error pipe (%s)", strerror(errno));
    }

    // wait for process

    for (auto target : this->targets) {
        target->proxy_pid = 0;
        target->target_pid = 0;
    }

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
