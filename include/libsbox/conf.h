/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_CONF_H
#define LIBSBOX_CONF_H

#include <unistd.h>

namespace libsbox {
    const uid_t uid = 31313;
    const gid_t gid = 31313;
    const int pipe_size = 10 * 1024 * 1024;
    const int clone_stack_size = 1024 * 1024;
} // namespace libsbox

#endif //LIBSBOX_CONF_H
