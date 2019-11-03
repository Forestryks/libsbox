/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_DAEMON_H
#define LIBSBOX_DAEMON_H

#include "context_manager.h"
#include "shared_id_getter.h"
#include "worker.h"

#include <string>
#include <memory>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

class Daemon : public ContextManager {
public:
    [[noreturn]]
    void run();

    static Daemon &get();

    [[noreturn]]
    void die(const std::string &error) override;
    void terminate() override;

    void report_error(const std::string &error);
    void close_error_pipe_read_end();
    int get_error_pipe_fd();
private:
    static Daemon daemon_;

    Daemon() = default;

    fs::path socket_path_;
    int server_socket_fd_{};
    std::unique_ptr<SharedIdGetter> id_getter_;
    volatile bool terminated_ = false;
    int num_boxes_{};
    int error_pipe_[2]{};
    std::vector<std::unique_ptr<Worker>> workers_;

    void prepare();
    void log(const std::string &error);
    void die_with_worker_status(int status);
};

#endif //LIBSBOX_DAEMON_H
