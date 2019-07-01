/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_DIE_H
#define LIBSBOX_DIE_H

#include <stdarg.h>

namespace libsbox {
    [[noreturn]] __attribute__((format(printf, 1, 2)))
    void die(const char *, ...);
} // namespace libsbox

#endif //LIBSBOX_DIE_H
