#include "libsbox.h"

#include <iostream>

namespace libsbox {
    execution_context::execution_context() {
        if (!registered_contexts.insert(this).second) {
            die("Failed to register context");
        }
    }

    execution_context::~execution_context() {
        if (registered_contexts.erase(this) == 0) {
            die("Failed to unregister context");
        }
    }

    template<bool critical>
    void die(const char *msg, ...) {
        const int ERR_BUF_SIZE = 2048;
        char err_buf[ERR_BUF_SIZE];
        va_list va_args;
        va_start(va_args, msg);
        vsnprintf(err_buf, ERR_BUF_SIZE, msg, va_args);
        va_end(va_args);

        printf("%s\n", err_buf);

        exit(-1);
    }

    std::set<execution_context *> registered_contexts;
}
