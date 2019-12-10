/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_ERROR_H
#define LIBSBOX_ERROR_H

#include <string>

class Error {
public:
    Error() = default;
    explicit Error(const std::string &msg);

    explicit operator bool() const;
    const std::string &get() const;
    void set(const std::string &msg);

private:
    std::string error_;
};

#endif //LIBSBOX_ERROR_H
