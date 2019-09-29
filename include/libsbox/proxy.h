/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_PROXY_H_
#define LIBSBOX_PROXY_H_

#include <libsbox/context.h>

#include <sched.h>

class Proxy : Context {
public:
    static Proxy &get();
    static pid_t spawn();

    [[noreturn]]
    void die(const std::string &error) override;
private:
    static Proxy proxy_;

    int server_socket_fd_ = -1;
    int socket_fd_ = -1;

    [[noreturn]]
    void serve();

    void prepare();
    void cleanup();
};

#endif //LIBSBOX_PROXY_H_
