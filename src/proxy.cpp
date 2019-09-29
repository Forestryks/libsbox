/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include <libsbox/proxy.h>
#include <libsbox/signals.h>
#include <libsbox/daemon.h>
#include <libsbox/utils.h>

#include <iostream>
#include <unistd.h>
#include <syslog.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <cstring>
#include <algorithm>

Proxy Proxy::proxy_;

Proxy &Proxy::get() {
    return proxy_;
}

pid_t Proxy::spawn() {
    pid_t pid = fork();
    if (pid != 0) {
        // In parent we just return, so spawn() just returns like fork()
        return pid;
    }

    Proxy::get().serve();
    Proxy::get().die("We should not get there");
}

[[noreturn]]
void Proxy::serve() {
    server_socket_fd_ = Daemon::get().get_server_socket_fd();

    prepare();

    while (interrupt_signal != SIGTERM) {
        socket_fd_ = accept(server_socket_fd_, nullptr, nullptr);
        if (socket_fd_ < 0) {
            if (errno == EINTR && interrupt_signal == SIGTERM) {
                continue;
            }
            die(format("Failed to accept connection: %m"));
        }

        std::cout << "Connection accepted" << std::endl;

        bool data_received = true;
        std::string data;
        while (true) {
            char buf[1024];
            int bytes_read = recv(socket_fd_, buf, sizeof(buf) - 1, 0);
            if (bytes_read < 0) {
                if (errno == EINTR && interrupt_signal == SIGTERM) {
                    data_received = false;
                    break;
                }
                die(format("Failed to receive data: %m"));
            }
            if (bytes_read == 0) {
                die("Request must end with \\\\0");
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

        std::cout << data << std::endl;
        std::reverse(data.begin(), data.end());

        if (write(socket_fd_, data.c_str(), data.size()) != (int) data.size()) {
            die(format("Cannot send response: %m"));
        }

        if (close(socket_fd_) != 0) {
            die(format("Cannot clone socket: %m"));
        }
    }

    cleanup();
    exit(0);
}

void Proxy::prepare() {
    Context::set_context(this);
}

void Proxy::cleanup() {

}

[[noreturn]]
void Proxy::die(const std::string &error) {
    syslog(LOG_ERR, "%s", ("[proxy] " + error).c_str());
    exit(1);
}
