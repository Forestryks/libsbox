/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_DAEMON_H
#define LIBSBOX_DAEMON_H

#include <libsbox/context.h>
#include <libsbox/shared_counter.h>

#include <string>
#include <vector>

class Daemon : Context {
public:
    static Daemon &get();

    [[noreturn]]
    void run();

    [[noreturn]]
    void die(const std::string &error) override;

    [[nodiscard]]
    int get_server_socket_fd() const;

    [[nodiscard]]
    SharedCounter *get_uid_counter() const;
private:
    static Daemon daemon_;

    std::string socket_path_;
    int server_socket_fd_;
    int num_boxes_;
    std::vector<pid_t> proxy_pids_;
    SharedCounter *uid_counter_;

    void prepare();
    void cleanup();
};

#endif //LIBSBOX_DAEMON_H
