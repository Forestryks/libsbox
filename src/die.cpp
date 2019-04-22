#include <libsbox/die.h>

#include <cstdlib>
#include <cstdio>

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

    printf("%s\n", err_buf);

    exit(-1);
}