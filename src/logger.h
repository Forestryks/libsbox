/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_LOGGER_H
#define LIBSBOX_LOGGER_H

#include "libsbox_internal.h"

#include <string>

class Logger {
public:
    static Logger &get();
    void _log(std::string msg);
    fd_t get_fd() const;
    static void init();
private:
    static Logger *logger_;
    Logger();

    fd_t fd_;
};

#define log(msg) Logger::get()._log(msg)

#endif //LIBSBOX_LOGGER_H
