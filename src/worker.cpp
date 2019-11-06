/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include "worker.h"
#include "daemon.h"
#include "signals.h"

#include <unistd.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/wait.h>

Worker *Worker::worker_ = nullptr;

Worker::Worker(int server_socket_fd, SharedIdGetter *id_getter)
    : server_socket_fd_(server_socket_fd), id_getter_(id_getter) {}

Worker &Worker::get() {
    return *worker_;
}

pid_t Worker::start() {
    pid_ = fork();
    if (pid_ != 0) {
        return pid_;
    }

    serve();
}

void Worker::_die(const std::string &error) {
    Daemon::get().report_error("[worker] " + error);
    _exit(1);
}

void Worker::terminate() {
    terminated_ = true;
}

pid_t Worker::get_pid() const {
    return pid_;
}

void sigchld_action_wrapper(int, siginfo_t *siginfo, void *) {
    Worker::get().sigchld_action(siginfo);
}

void Worker::serve() {
    worker_ = this;
    ContextManager::set(this);

    Daemon::get().close_error_pipe_read_end();

    // Worker will die if parent exits
    if (prctl(PR_SET_PDEATHSIG, SIGKILL) != 0) {
        die(format("Cannot set parent death signal: %m"));
    }
    // To prevent race condition
    if (getppid() == 0) {
        raise(SIGKILL);
    }

    // We need check containers' exit codes asynchronously to avoid deadlocks
    struct sigaction action{};
    action.sa_sigaction = sigchld_action_wrapper;
    action.sa_flags = (SA_SIGINFO | SA_RESTART);
    if (sigaction(SIGCHLD, &action, nullptr) != 0) {
        die(format("Failed to set sigaction to SIGCHLD: %m"));
    }

    while (!terminated_) {
        // If worker is terminated we want accept() to be interrupted
        set_standard_handler_restart(SIGTERM, false);

        // Accept connection on socket
        socket_fd_ = accept(server_socket_fd_, nullptr, nullptr);
        if (socket_fd_ < 0) {
            if (terminated_) {
                break;
            }
            die(format("Failed to accept connection: %m"));
        }

        // If worker is terminated we want to complete current request, so we don't want to interrupt anything
        set_standard_handler_restart(SIGTERM, true);

        // Read from socket until end-of-file or null-byte
        std::string request;
        while (true) {
            char buf[1024];
            int bytes_read = recv(socket_fd_, buf, sizeof(buf) - 1, 0);
            if (bytes_read < 0) {
                die(format("Failed to receive data: %m"));
            }
            if (bytes_read == 0) {
                break;
            }

            buf[bytes_read] = 0;
            request += buf;

            if ((int) strlen(buf) < bytes_read) {
                // null-byte occurs in buf
                break;
            }
        }

        std::string response = process(request);

        if (write(socket_fd_, response.c_str(), response.size()) != (int) response.size()) {
            die(format("Cannot send response: %m"));
        }

        if (close(socket_fd_) != 0) {
            die(format("Cannot close socket: %m"));
        }
    }

    _exit(0);
}

std::string Worker::process(const std::string &request) {
    nlohmann::json json_request = parse_json(request);

    prepare_containers(json_request);
    // To start execution we must wait for worker + (all containers)
    run_start_barrier_.reset((int) containers_.size() + 1);
    write_tasks(json_request);
    run_tasks();
    close_pipes();

    return collect_results();
}

nlohmann::json Worker::parse_json(const std::string &request) {
    nlohmann::json json_object;
    try {
        json_object = nlohmann::json::parse(request);
    } catch (nlohmann::json::exception &e) {
        die(format("Failed to parse json: %s", e.what()));
    }
    return json_object;
}

void Worker::prepare_containers(const nlohmann::json &json_request) {
    if (!json_request.is_object()) {
        die(format("Received json is not an object"));
    }

    try {
        const nlohmann::json &json_tasks = json_request.at("tasks");
        int next_persistent_container = 0;
        for (const auto &json_task : json_tasks) {
            bool persistence_allowed = true;
            if (json_task.at("ipc").get<bool>() || !json_task.at("standard_binds").get<bool>()) {
                persistence_allowed = false;
            }

            Container *created_container = nullptr;
            if (persistence_allowed) {
                if (next_persistent_container == (int) persistent_containers_.size()) {
                    created_container = new Container(id_getter_->get(), true);
                    persistent_containers_.emplace_back(created_container);
                }
                containers_.push_back(persistent_containers_[next_persistent_container].get());
                next_persistent_container++;
            } else {
                created_container = new Container(id_getter_->get(), false);
                temporary_containers_.emplace_back(created_container);
                containers_.push_back(created_container);
            }

            if (created_container != nullptr) {
                if (created_container->start() < 0) {
                    die(format("Cannot start() container: %m"));
                }
            }
        }
    } catch (const nlohmann::json::exception &e) {
        die(format("Failed to parse request: %s", e.what()));
    }
}

void Worker::write_tasks(const nlohmann::json &json_request) {
    try {
        const nlohmann::json &json_tasks = json_request.at("tasks");
        for (int i = 0; i < (int) json_tasks.size(); ++i) {
            containers_[i]->parse_task_from_json(json_tasks[i]);
        }
    } catch (const nlohmann::json::exception &e) {
        die(format("Failed to parse request: %s", e.what()));
    }
}

void Worker::run_tasks() {
    // Containers are wait()ing for tasks on their barriers
    for (auto *container : containers_) {
        container->get_barrier()->wait();
    }
    // Wait for run start
    run_start_barrier_.wait();
}

std::string Worker::collect_results() {
    std::string result;
    nlohmann::json json_result;
    try {
        json_result["tasks"] = nlohmann::json::array();
        // Containers will wait() on their barriers when results are ready
        for (auto *container : containers_) {
            container->get_barrier()->wait();
            json_result["tasks"].push_back(container->results_to_json());
        }

        result = json_result.dump();
    } catch (nlohmann::json::exception &e) {
        die(format("Failed to serialize results: %s", e.what()));
    }

    for (auto &container : temporary_containers_) {
        id_getter_->put(container->get_id());
        int status;
        pid_t pid = waitpid(container->get_pid(), &status, 0);
        if (pid < 0) {
            die(format("Cannot wait() for temporary container: %m"));
        }
        // We don't need to check status here, it was checked in sigaction
    }
    temporary_containers_.clear();
    containers_.clear();

    return result;
}

void Worker::sigchld_action(siginfo_t *siginfo) {
    if (siginfo->si_code != CLD_EXITED || siginfo->si_status != 0) {
        if (siginfo->si_code == CLD_EXITED) {
            die(format("Container exited with exit code %d", siginfo->si_status));
        } else {
            die(format("Container received signal %d (%s)", siginfo->si_status, strsignal(siginfo->si_status)));
        }
    }
}

std::pair<int, int> Worker::get_pipe(const std::string &pipe_name) {
    if (pipes_.find(pipe_name) == pipes_.end()) {
        int fd[2];
        if (pipe(fd) != 0) {
            die(format("Cannot create pipe: %m"));
        }
        pipes_[pipe_name] = {fd[0], fd[1]};
    }

    return pipes_[pipe_name];
}

void Worker::close_pipes() {
    for (auto &entry : pipes_) {
        if (close(entry.second.first) != 0) {
            die(format("Cannot close read end of pipe: %m"));
        }
        if (close(entry.second.second) != 0) {
            die(format("Cannot close write end of pipe: %m"));
        }
    }
    pipes_.clear();
}

SharedBarrier *Worker::get_run_start_barrier() {
    return &run_start_barrier_;
}
