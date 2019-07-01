/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include <libsbox/die.h>
#include <libsbox/execution_context.h>
#include <libsbox/conf.h>
#include <libsbox/logger.h>

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>

[[noreturn]] __attribute__((format(printf, 1, 2)))
void libsbox::die(const char *msg, ...) {
    va_list va_args;
    char err_buf[err_buf_size];
    va_start(va_args, msg);
    vsnprintf(err_buf, err_buf_size, msg, va_args);

    if (current_target == nullptr) {
        // We are in invoker process
        error(err_buf);
        if (current_context != nullptr) current_context->die();
    } else {
        char err_buf2[err_buf_size + 9];
        if (current_target->inside_box) {
            // We are in slave
            snprintf(err_buf2, err_buf_size + 9, "[slave] %s", err_buf);
        } else {
            // We are in proxy
            snprintf(err_buf2, err_buf_size + 9, "[proxy] %s", err_buf);
        }

        write(current_context->error_pipe[1], err_buf2, strlen(err_buf2));
        exit(1);
    }

    exit(1);
}
