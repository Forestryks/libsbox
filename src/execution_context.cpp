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
#include <sys/time.h>

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

    if (pipe2(this->error_pipe, O_NONBLOCK | O_DIRECT | O_CLOEXEC) != 0) {
        libsbox::die("Cannot create error pipe: pipe2 failed (%s)", strerror(errno));
    }

    for (auto target : this->targets) {
        if (pipe2(target->status_pipe, O_CLOEXEC) != 0) {
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

void libsbox::execution_context::reset_wall_clock() {
    gettimeofday(&this->run_start, nullptr);
}

long libsbox::execution_context::get_wall_clock() {
    struct timeval now = {}, seg = {};
    gettimeofday(&now, nullptr);
    timersub(&now, &this->run_start, &seg);
    return seg.tv_sec * 1000 + seg.tv_usec / 1000;
}

void libsbox::execution_context::run() {
    current_context = this;

    for (auto target : this->targets) {
        target->prepare();
    }

    this->create_pipes();

    for (auto target : this->targets) {
        target->start_proxy();
    }

    if (close(this->error_pipe[1]) != 0) {
        libsbox::die("Cannot close write end of error pipe (%s)", strerror(errno));
    }

    this->reset_wall_clock();
    start_timer(timer_interval);

    int exited_cnt = 0;
    char err_buf[err_buf_size];
    while (exited_cnt < (int)this->targets.size()) {
        int stat;
        pid_t pid = wait(&stat);
        if (pid < 0) {
            if (errno != EINTR) {
                libsbox::die("Wait failed (%s)", strerror(errno));
            }
            if (interrupt_signal != SIGALRM) {
                libsbox::die("Wait interrupted with %s", strsignal(interrupt_signal));
            }

            if (this->wall_time_limit != -1 && this->get_wall_clock() > this->wall_time_limit) {
                for (auto target : this->targets) {
                    if (!target->running) continue;
                    target->wall_time_limit_exceeded = true;
                    kill(-target->proxy_pid, SIGKILL);
                    kill(target->proxy_pid, SIGKILL);
                    target->proxy_killed = true;
                }
            }

            for (auto target : this->targets) {
                if (!target->running) continue;
                if (target->time_limit != -1 && target->get_time_usage() > target->time_limit) {
                    target->time_limit_exceeded = true;
                    kill(-target->proxy_pid, SIGKILL);
                    kill(target->proxy_pid, SIGKILL);
                    target->proxy_killed = true;
                }
            }

            continue;
        }

        int size = read(error_pipe[0], err_buf, err_buf_size - 1);
        if (size > 0) {
            err_buf[size] = 0;
            libsbox::die("%s", err_buf);
        }

        execution_target *exited_target = nullptr;
        for (auto target : this->targets) {
            if (target->proxy_pid == pid) {
                exited_target = target;
                exited_cnt++;
                break;
            }
        }

        if (exited_target == nullptr) {
            libsbox::die("Unknown child exited");
        }

        exited_target->running = false;

        if (!exited_target->proxy_killed) {
            size = read(exited_target->status_pipe[0], &stat, sizeof(stat));
            if (size != sizeof(stat)) {
                libsbox::die("Cannot recieve target exit status from proxy (%s)", strerror(errno));
            }

            if (WIFEXITED(stat)) {
                exited_target->exited = true;
                exited_target->exit_code = WEXITSTATUS(stat);
            }
            if (WIFSIGNALED(stat)) {
                exited_target->signaled = true;
                exited_target->term_signal = WTERMSIG(stat);
            }
        }

        exited_target->time_usage_wall = this->get_wall_clock();
        exited_target->time_usage = exited_target->get_time_usage();
        exited_target->time_usage_sys = exited_target->get_time_usage_sys();
        exited_target->time_usage_user = exited_target->get_time_usage_user();
        exited_target->memory_usage = exited_target->get_memory_usage();
        exited_target->oom_killed = exited_target->get_oom_status();
    }

    stop_timer();

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
