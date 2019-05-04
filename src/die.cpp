/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include <libsbox/die.h>
#include <libsbox/execution_context.h>

#include <cstdio>
#include <cstring>
#include <cstdlib>

void standard_fatal_handler(const char *msg) {
    fprintf(stderr, "%s", msg);
}

void (*libsbox::fatal_handler)(const char *) = standard_fatal_handler;

[[noreturn]] __attribute__((format(printf, 1, 2)))
void libsbox::die(const char *msg, ...) {
    va_list va_args;
    va_start(va_args, msg);
    die(false, msg, va_args);
}

[[noreturn]] __attribute__((format(printf, 1, 2)))
void libsbox::panic(const char *msg, ...) {
    va_list va_args;
    va_start(va_args, msg);
    die(true, msg, va_args);
}

[[noreturn]]
void libsbox::die(bool critical, const char *msg, va_list va_args) {
    const int ERR_BUF_SIZE = 2048;
    char err_buf[ERR_BUF_SIZE];
    vsnprintf(err_buf, ERR_BUF_SIZE, msg, va_args);
    if (critical) {
        char tmp_buf[ERR_BUF_SIZE];
        snprintf(tmp_buf, ERR_BUF_SIZE, "Critical error! %s", err_buf);
        strcpy(err_buf, tmp_buf);
    }

    fatal_handler(err_buf);

    if (current_context != nullptr) current_context->die();

    if (critical) exit(-1);
    exit(-2);
}
