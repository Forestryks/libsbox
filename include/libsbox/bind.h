/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_BIND_H
#define LIBSBOX_BIND_H

#include <string>

namespace libsbox {
    enum {
        BIND_READWRITE = 1,
        BIND_OPTIONAL = 2,
        BIND_COPY_IN = 0,
        BIND_COPY_OUT = 0
    };

    struct bind_rule {
        std::string inside;
        std::string outside;
        int flags;
    };

//    struct bind_rule_old {
//        std::string path;
//        int flags;
//    };
} // namespace libsbox

#endif //LIBSBOX_BIND_H
