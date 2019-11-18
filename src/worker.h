/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_WORKER_H
#define LIBSBOX_WORKER_H

#include "context_manager.h"
#include "shared_id_getter.h"
#include "shared_barrier.h"
#include "container.h"
#include "schema_validator.h"

#include <sys/signal.h>
#include <map>

class Worker final : public ContextManager {
public:
    Worker(fd_t server_socket_fd, SharedIdGetter *id_getter);

    static Worker &get();
    pid_t start();

    [[noreturn]]
    void _die(const std::string &error) override;
    void terminate() override;

    pid_t get_pid() const;

    std::pair<fd_t, fd_t> get_pipe(const std::string &pipe_name);
    SharedBarrier *get_run_start_barrier();
private:
    static Worker *worker_;
    fd_t server_socket_fd_;
    fd_t socket_fd_ = -1;
    SharedIdGetter *id_getter_;
    SharedBarrier run_start_barrier_{1};
    pid_t pid_{-1};
    std::vector<libsbox::Task *> tasks_;

    volatile bool terminated_ = false;

    std::vector<std::unique_ptr<Container>> permanent_containers_;
    std::vector<std::unique_ptr<Container>> temporary_containers_;
    std::vector<Container *> containers_;
    std::unique_ptr<SchemaValidator> request_validator_;

    std::map<std::string, std::pair<fd_t, fd_t>> pipes_;
    void close_pipes();

    [[noreturn]]
    void serve();
    std::string process(const std::string &request);
    Error parse_and_validate_json_request(const std::string &request);
    void prepare_containers();
    void write_tasks();
    void run_tasks();
    std::string collect_results();

    static void sigchld_action(int, siginfo_t *siginfo, void *);
};

#endif //LIBSBOX_WORKER_H
