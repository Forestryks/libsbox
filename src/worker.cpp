/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include <libsbox/worker.h>
#include <libsbox/daemon.h>

#include <unistd.h>
#include <syslog.h>
#include <sys/socket.h>

Worker Worker::worker_;

Worker &Worker::get() {
    return worker_;
}

pid_t Worker::spawn() {
    pid_t pid = fork();
    if (pid != 0) {
        // In parent we just return, so spawn() just returns like fork()
        return pid;
    }

    Worker::get().serve();
    Worker::get().die("We should not get there");
}

[[noreturn]]
void Worker::die(const std::string &error) {
    syslog(LOG_ERR, "%s", ("[worker] " + error).c_str());
    exit(1);
}

void Worker::terminate() {
    terminated_ = true;
}

[[noreturn]]
void Worker::serve() {
    ContextManager::set(this);
    server_socket_fd_ = Daemon::get().get_server_socket_fd();

    while (!terminated_) {
        socket_fd_ = accept(server_socket_fd_, nullptr, nullptr);
        if (socket_fd_ < 0) {
            if (terminated_) {
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
                if (terminated_) {
                    data_received = false;
                    break;
                }
                die(format("Failed to receive data: %m"));
            }
            if (bytes_read == 0) {
                // die("Request must end with \\0");
                break;
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

std::string Worker::process(const std::string &request) {
    nlohmann::json json_request = parse_json(request);
    prepare_containers(json_request);

    run_start_waiter_.wait(tasks_cnt_);

    close_pipes();

    run_end_waiter_.wait(tasks_cnt_);

    return get_results();
}

nlohmann::json Worker::parse_json(const std::string &request) {
    nlohmann::json json_object;
    try {
        json_object = nlohmann::json::parse(request);
    } catch (nlohmann::json::exception &e) {
        die(format("Failed to parse request: %s", e.what()));
    }

    return json_object;
}

void Worker::prepare_containers(const nlohmann::json &json_request) {
    if (!json_request.is_object()) {
        die(format("Received json is not an object"));
    }

    assert(pipes_.empty());

    try {
        const nlohmann::json &json_tasks = json_request.at("tasks");
        tasks_cnt_ = json_tasks.size();
        while ((int) containers_.size() < tasks_cnt_) {
            containers_.push_back(new Container(Daemon::get().get_uid_counter()->get_and_inc()));
            if (containers_.back()->start() < 0) {
                die(format("Cannot fork() container: %m"));
            }
        }

        for (int id = 0; id < tasks_cnt_; ++id) {
            containers_[id]->get_task_data_from_json(json_tasks.at(id));
        }
    } catch (const nlohmann::json::exception &e) {
        die(format("Failed to parse request: %s", e.what()));
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
}

std::string Worker::get_results() {
    nlohmann::json result;
    try {
        result["tasks"] = nlohmann::json::array();
        for (int id = 0; id < (int) tasks_cnt_; ++id) {
            result["tasks"].push_back(containers_[id]->results_to_json());
        }

        return result.dump();
    } catch (nlohmann::json::exception &e) {
        die(format("Failed to serialize result: %s", e.what()));
    }
    exit(-1); // we should not get here
}

SharedWaiter & Worker::get_run_start_waiter() {
    return run_start_waiter_;
}

SharedWaiter & Worker::get_run_end_waiter() {
    return run_end_waiter_;
}
