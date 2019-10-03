/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include <libsbox/proxy.h>
#include <libsbox/signals.h>
#include <libsbox/daemon.h>
#include <libsbox/utils.h>

#include <iostream>
#include <unistd.h>
#include <syslog.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <cstring>
#include <algorithm>
#include <json.hpp>
#include <libsbox/shared_memory.h>

Proxy *Proxy::proxy_;

Proxy &Proxy::get() {
    return *proxy_;
}

pid_t Proxy::spawn() {
    pid_t pid = fork();
    if (pid != 0) {
        // In parent we just return, so spawn() just returns like fork()
        return pid;
    }

    proxy_ = new Proxy();

    Proxy::get().serve();
    Proxy::get().die("We should not get there");
}

[[noreturn]]
void Proxy::serve() {
    Context::set_context(this);
    server_socket_fd_ = Daemon::get().get_server_socket_fd();

    while (interrupt_signal != SIGTERM) {
        socket_fd_ = accept(server_socket_fd_, nullptr, nullptr);
        if (socket_fd_ < 0) {
            if (errno == EINTR && interrupt_signal == SIGTERM) {
                continue;
            }
            die(format("Failed to accept connection: %m"));
        }

        // read from socket until null-byte
        bool data_received = true;
        std::string data;
        while (true) {
            char buf[1024];
            int bytes_read = recv(socket_fd_, buf, sizeof(buf) - 1, 0);
            if (bytes_read < 0) {
                if (errno == EINTR && interrupt_signal == SIGTERM) {
                    data_received = false;
                    break;
                }
                die(format("Failed to receive data: %m"));
            }
            if (bytes_read == 0) {
                die("Request must end with \\0");
            }

            buf[bytes_read] = 0;

            data += buf;

            if ((int) strlen(buf) < bytes_read) {
                // null-byte character occurs in buf
                break;
            }
        }

        if (!data_received) {
            close(socket_fd_);
            continue;
        }

        std::string response = process(data);

        if (write(socket_fd_, response.c_str(), response.size()) != (int) response.size()) {
            die(format("Cannot send response: %m"));
        }

        if (close(socket_fd_) != 0) {
            die(format("Cannot clone socket: %m"));
        }
    }

    exit(0);
}

[[noreturn]]
void Proxy::die(const std::string &error) {
    syslog(LOG_ERR, "%s", ("[proxy] " + error).c_str());
    exit(1);
}

std::string Proxy::process(const std::string &data) {
    parse_request(data);
    // Create pipes, they will be visible for all workers
    create_pipes();
    // Ensure that we have enough workers
    prepare_workers();
    // Write target descriptors
    write_target_descriptors();

    start_barrier_.wait(targets_.size());

    // Destroy pipes, we don't need them anymore in proxy and workers
    destroy_pipes();

    end_barrier_.wait(targets_.size());

    return read_target_descriptors();
}

void Proxy::parse_request(const std::string &request) {
    nlohmann::json json_object;
    try {
        json_object = nlohmann::json::parse(request);
    } catch (nlohmann::json::exception &e) {
        die(format("Failed to parse request: %s", e.what()));
    }

    pipes_.clear();
    targets_.clear();

    try {
        if (!json_object["targets"].is_array()) {
            die(format("Failed to parse request: \"targets\" is not array"));
        }
        for (const auto &target : json_object["targets"]) {
            targets_.push_back(std::make_unique<Target>(target));
        }
    } catch (nlohmann::json::exception &e) {
        die(format("Failed to parse request: %s", e.what()));
    }

    // If stream output file begins with '@' it is pipe name, so we need to create it
    for (auto &it : targets_) {
        if (it->get_stdin()[0] == '@') {
            pipes_[it->get_stdin().substr(1)] = {};
        }
        if (it->get_stdout()[0] == '@') {
            pipes_[it->get_stdout().substr(1)] = {};
        }
        if (it->get_stderr()[0] == '@') {
            pipes_[it->get_stderr().substr(1)] = {};
        }
    }
}

void Proxy::create_pipes() {
    for (auto &entry : pipes_) {
        int fd[2];
        if (pipe(fd) != 0) {
            die(format("Cannot create pipe: %m"));
        }
        entry.second = {fd[0], fd[1]};
    }
}

void Proxy::destroy_pipes() {
    for (auto &entry : pipes_) {
        if (close(entry.second.first) != 0) {
            die(format("Cannot close read end of pipe: %m"));
        }
        if (close(entry.second.second) != 0) {
            die(format("Cannot close write end of pipe: %m"));
        }
    }
}

void Proxy::prepare_workers() {
    while (workers_.size() < targets_.size()) {
        workers_.push_back(std::make_unique<Worker>(Daemon::get().get_uid_counter()->get_and_inc()));
        workers_.back()->start();
    }
}

void Proxy::write_target_descriptors() {
    for (int id = 0; id < (int) targets_.size(); ++id) {
        Target *target = targets_[id].get();
        Worker *worker = workers_[id].get();
        target_desc *desc = worker->get_target_desc();

        desc->time_limit_ms = target->get_time_limit_ms();
        desc->wall_time_limit_ms = target->get_wall_time_limit_ms();
        desc->memory_limit_kb = target->get_memory_limit_kb();
        desc->fsize_limit_kb = target->get_fsize_limit_kb();
        desc->max_files = target->get_max_files();
        desc->max_threads = target->get_max_threads();

        std::string stdin_filename = target->get_stdin();
        desc->stdin_desc.fd = -1;
        desc->stdin_desc.filename[0] = '\0';
        if (stdin_filename[0] == '@') {
            desc->stdin_desc.fd = pipes_[stdin_filename.substr(1)].first;
        } else if (stdin_filename != "-") {
            strcpy(desc->stdin_desc.filename, stdin_filename.c_str());
        }

        std::string stdout_filename = target->get_stdout();
        desc->stdout_desc.fd = -1;
        desc->stdout_desc.filename[0] = '\0';
        if (stdout_filename[0] == '@') {
            desc->stdout_desc.fd = pipes_[stdout_filename.substr(1)].second;
        } else if (stdout_filename != "-") {
            strcpy(desc->stdout_desc.filename, stdout_filename.c_str());
        }

        std::string stderr_filename = target->get_stderr();
        desc->stderr_desc.fd = -1;
        desc->stderr_desc.filename[0] = '\0';
        if (stderr_filename[0] == '@') {
            desc->stderr_desc.fd = pipes_[stderr_filename.substr(1)].second;
        } else if (stderr_filename != "-") {
            strcpy(desc->stderr_desc.filename, stderr_filename.c_str());
        }

        const auto &argv = target->get_argv();
        int ptr = 0;
        for (int i = 0; i < (int) argv.size(); ++i) {
            desc->argv[i] = desc->argv_data + ptr;
            strcpy(desc->argv_data + ptr, argv[i].c_str());
            ptr += (int) argv[i].size() + 1;
        }
        desc->argv[(int) argv.size()] = nullptr;

        // TODO: environment is not supported yet
        desc->env[0] = nullptr;

        const auto &binds = target->get_binds();
        desc->bind_count = binds.size();
        for (int i = 0; i < (int) binds.size(); ++i) {
            strcpy(desc->binds[i].inside, binds[i].get_inside().c_str());
            strcpy(desc->binds[i].outside, binds[i].get_outside().c_str());
            desc->binds[i].flags = binds[i].get_flags();
        }

        desc->time_usage_ms = -1;
        desc->time_usage_sys_ms = -1;
        desc->time_usage_user_ms = -1;
        desc->wall_time_usage_ms = -1;
        desc->memory_usage_kb = -1;

        desc->time_limit_exceeded = false;
        desc->wall_time_limit_exceeded = false;
        desc->exited = false;
        desc->exit_code = -1;
        desc->signaled = false;
        desc->term_signal = -1;
        desc->oom_killed = false;

        desc->error = false;

        workers_[id]->get_desc_write_barrier()->sub();
    }
}

SharedBarrier &Proxy::get_start_barrier() {
    return start_barrier_;
}

SharedBarrier &Proxy::get_end_barrier() {
    return end_barrier_;
}

std::string Proxy::read_target_descriptors() {
    nlohmann::json result;
    try {
        result["targets"] = nlohmann::json::array();
        for (int id = 0; id < (int) targets_.size(); ++id) {
            target_desc *desc = workers_[id].get()->get_target_desc();
            result["targets"].push_back(nlohmann::json::object({
                {"time_usage_ms", desc->time_usage_ms},
                {"time_usage_sys_ms", desc->time_usage_sys_ms},
                {"time_usage_user_ms", desc->time_usage_user_ms},
                {"wall_time_usage_ms", desc->wall_time_usage_ms},
                {"memory_usage_kb", desc->memory_usage_kb},
                {"time_limit_exceeded", desc->time_limit_exceeded},
                {"wall_time_limit_exceeded", desc->wall_time_limit_exceeded},
                {"exited", desc->exited},
                {"exit_code", desc->exit_code},
                {"signaled", desc->signaled},
                {"term_signal", desc->term_signal},
                {"oom_killed", desc->oom_killed}
            }));
        }

        return result.dump();
    } catch (nlohmann::json::exception &e) {
        die(format("Failed to serialize result: %s", e.what()));
    }
}
