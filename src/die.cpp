#include <libsbox/die.h>
#include <libsbox/fs.h>
#include <libsbox/init.h>

#include <cstdlib>
#include <cstdio>
#include <cstring>

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
        snprintf(tmp_buf, ERR_BUF_SIZE, "Critical error, cleanup impossible! %s", err_buf);
        strcpy(err_buf, tmp_buf);
    }

    fatal_handler(err_buf);

    if (critical) exit(-1);

    rmtree(join_path(prefix, id), false);
}
