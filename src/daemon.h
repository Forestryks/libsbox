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

class Daemon final : public ContextManager {
public:
    Daemon() = default;

    [[noreturn]]
    void run();

    static Daemon &get();

    [[noreturn]]
    void _die(const std::string &error) override;
    void terminate() override;
private:
    static Daemon *daemon_;

    fs::path socket_path_;
    fd_t server_socket_fd_{};
    std::unique_ptr<SharedIdGetter> id_getter_;
    volatile bool terminated_ = false;
    uint32_t num_boxes_{};
    std::vector<std::unique_ptr<Worker>> workers_;

    void prepare();
    void die_with_worker_status(int status);
};

#endif //LIBSBOX_DAEMON_H
