/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_PROXY_H_
#define LIBSBOX_PROXY_H_

#include <libsbox/shared_barrier.h>
#include <libsbox/context.h>
#include <libsbox/target.h>
#include <libsbox/worker.h>

#include <sched.h>
#include <map>
#include <vector>
#include <memory>

class Proxy : Context {
public:
    static Proxy &get();
    static pid_t spawn();

    [[noreturn]]
    void die(const std::string &error) override;

    SharedBarrier &get_start_barrier();
    SharedBarrier &get_end_barrier();
private:
    static Proxy *proxy_;

    int server_socket_fd_ = -1;
    int socket_fd_ = -1;

    std::map<std::string, std::pair<int, int>> pipes_;
    std::vector<std::unique_ptr<Target>> targets_;

    SharedBarrier start_barrier_, end_barrier_;

    std::vector<std::unique_ptr<Worker>> workers_;

    [[noreturn]]
    void serve();

    std::string process(const std::string &data);

    void parse_request(const std::string &request);

    void create_pipes();
    void destroy_pipes();
    void prepare_workers();
    void write_target_descriptors();
    std::string read_target_descriptors();
};

#endif //LIBSBOX_PROXY_H_
