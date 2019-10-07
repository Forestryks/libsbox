/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_WORKER_H_
#define LIBSBOX_WORKER_H_

#include <libsbox/context_manager.h>
#include <libsbox/container.h>

#include <map>
#include <json.hpp>

class Worker : public ContextManager {
public:
    static Worker &get();
    static pid_t spawn();

    [[noreturn]]
    void die(const std::string &error) override;
    void terminate() override;

    std::pair<int, int> get_pipe(const std::string &pipe_name);
    SharedWaiter & get_run_start_waiter();
    SharedWaiter & get_run_end_waiter();
private:
    static Worker worker_;

    volatile bool terminated_ = false;

    int server_socket_fd_;
    int socket_fd_;

    int tasks_cnt_;
    std::map<std::string, std::pair<int, int>> pipes_;
    std::vector<Container *> containers_;

    SharedWaiter run_start_waiter_;
    SharedWaiter run_end_waiter_;

    [[noreturn]]
    void serve();

    std::string process(const std::string &request);

    nlohmann::json parse_json(const std::string &request);
    void prepare_containers(const nlohmann::json &json_request);
    void close_pipes();
    std::string get_results();
};

#endif //LIBSBOX_WORKER_H_
