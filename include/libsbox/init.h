/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_INIT_H
#define LIBSBOX_INIT_H

namespace libsbox {
    extern bool initialized;

    void init(void (*fatal_error_handler)(const char *));

    void init_credentials();
} // namespace libsbox

#endif //LIBSBOX_INIT_H
