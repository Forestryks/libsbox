/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include "libsbox/error.h"

Error::Error(const std::string &msg) : error_(msg) {}

Error::operator bool() const {
    return !error_.empty();
}

const std::string &Error::get() const {
    return error_;
}

void Error::set(const std::string &msg) {
    if (error_.empty()) {
        error_ = msg;
    }
}
