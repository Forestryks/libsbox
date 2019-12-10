/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include "worker.h"
#include "signals.h"
#include "schema_validator.h"

#include <unistd.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "logger.h"
#include "generated/request_schema.h"

Worker *Worker::worker_ = nullptr;

Worker::Worker(fd_t server_socket_fd, SharedIdGetter *id_getter)
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
    log(error);
    _exit(1);
}

void Worker::terminate() {
    terminated_ = true;
}

pid_t Worker::get_pid() const {
    return pid_;
}

void Worker::serve() {
    worker_ = this;
    ContextManager::set(this, "worker");

    // Move from foreground group when libsboxd started from terminal
    if (setpgrp() != 0) {
        die("setpgrp() failed");
    }

    // Worker will die if parent exits
    if (prctl(PR_SET_PDEATHSIG, SIGKILL) != 0) {
        die(format("Cannot set parent death signal: %m"));
    }
    // To prevent race condition
    if (getppid() == 0) {
        raise(SIGKILL);
    }

    request_validator_ = std::make_unique<SchemaValidator>(request_schema_data);
    if (!request_validator_->get_error().empty()) {
        die(request_validator_->get_error());
    }

    // We need check containers' exit codes asynchronously to avoid deadlocks
    set_sigchld_action(sigchld_action);

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

            if (strlen(buf) < static_cast<size_t>(bytes_read)) {
                // null-byte occurs in buf
                break;
            }
        }

        std::string response = process(request);

        int cnt = write(socket_fd_, response.c_str(), response.size());
        if (cnt < 0 || static_cast<size_t>(cnt) != response.size()) {
            die(format("Cannot send response: %m"));
        }

        if (close(socket_fd_) != 0) {
            die(format("Cannot close socket: %m"));
        }
    }

    _exit(0);
}

std::string Worker::process(const std::string &request) {
    auto error = parse_and_validate_json_request(request);
    if (error) {
        rapidjson::StringBuffer string_buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(string_buffer);
        writer.StartObject();
        writer.Key("error");
        writer.String(error.get().c_str());
        writer.EndObject();
        return string_buffer.GetString();
    }

    prepare_containers();
    // To start execution we must wait for worker + all containers + all slaves
    run_start_barrier_.reset(containers_.size() * 2 + 1);
    write_tasks();
    run_tasks();
    close_pipes();

    return collect_results();
}

Error Worker::parse_and_validate_json_request(const std::string &request) {
    rapidjson::Document document;
    document.Parse(request.c_str());

    if (document.HasParseError()) {
        return Error(format(
            "Request JSON incorrect: %s (at %zi)",
            GetParseError_En(document.GetParseError()),
            document.GetErrorOffset()
        ));
    }

    if (!request_validator_->validate(document)) {
        return Error(request_validator_->get_error());
    }

    assert(tasks_.empty());

    for (const auto &json_task : document["tasks"].GetArray()) {
        libsbox::Task *task = new libsbox::Task();
        task->deserialize_request(json_task);
        tasks_.push_back(task);
    }

    return Error();
}

void Worker::prepare_containers() {
    size_t next_permanent_container = 0;
    for (auto task : tasks_) {
        bool permanent_container_allowed = true;
        if (task->get_need_ipc() || !task->get_use_standard_binds()) {
            permanent_container_allowed = false;
        }

        Container *created_container = nullptr;
        if (permanent_container_allowed) {
            if (next_permanent_container == permanent_containers_.size()) {
                created_container = new Container(id_getter_->get(), true);
                permanent_containers_.emplace_back(created_container);
            }
            containers_.push_back(permanent_containers_[next_permanent_container].get());
            next_permanent_container++;
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
}

void Worker::write_tasks() {
    for (size_t i = 0; i < tasks_.size(); ++i) {
        containers_[i]->set_task(tasks_[i]);
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
    for (size_t i = 0; i < tasks_.size(); ++i) {
        containers_[i]->get_barrier()->wait();
        containers_[i]->put_results(tasks_[i]);
    }

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

    writer.StartObject();
    writer.Key("tasks");
    writer.StartArray();
    for (auto task : tasks_) {
        task->serialize_response(writer);
        delete task;
    }
    writer.EndArray();
    writer.EndObject();
    tasks_.clear();

    std::string result = buffer.GetString();

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

void Worker::sigchld_action(int, siginfo_t *siginfo, void *) {
    if (siginfo->si_code != CLD_EXITED || siginfo->si_status != 0) {
        if (siginfo->si_code == CLD_EXITED) {
            die(format("Container exited with exit code %d", siginfo->si_status));
        } else {
            die(format("Container received signal %d (%s)", siginfo->si_status, strsignal(siginfo->si_status)));
        }
    }
}

std::pair<fd_t, fd_t> Worker::get_pipe(const std::string &pipe_name) {
    if (pipes_.find(pipe_name) == pipes_.end()) {
        fd_t fd[2];
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
