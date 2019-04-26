/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_BIND_H
#define LIBSBOX_BIND_H

#include <map>

namespace libsbox {
    enum {
        BIND_READWRITE = 1,
        BIND_ALLOWDEV = 2,
        BIND_NOREC = 4,
        BIND_OPTIONAL = 8,
        BIND_FILE = 16,
        BIND_COPY_IN = 32,
        BIND_COPY_OUT = 64
    };

    struct bind_rule {
        std::string path;
        int flags;
    };
} // namespace libsbox

#endif //LIBSBOX_BIND_H
