/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_DAEMON_H_
#define LIBSBOX_DAEMON_H_

#include <libsbox/context_manager.h>
#include <libsbox/shared_counter.h>

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

    [[nodiscard]] int get_server_socket_fd() const;
    [[nodiscard]] SharedCounter *get_uid_counter() const;
private:
    static Daemon daemon_;

    fs::path socket_path_;
    int server_socket_fd_;
    SharedCounter *uid_counter_;
    volatile bool terminated_ = false;
    int num_boxes_;
    std::vector<pid_t> workers_pids_;

    void prepare();
};

#endif //LIBSBOX_DAEMON_H_
