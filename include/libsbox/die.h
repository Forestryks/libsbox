#ifndef LIBSBOX_DIE_H
#define LIBSBOX_DIE_H

#include <stdarg.h>

namespace libsbox {
    extern void (*fatal_handler)(const char *);

    [[noreturn]] __attribute__((format(printf, 1, 2)))
    void die(const char *, ...);

    [[noreturn]] __attribute__((format(printf, 1, 2)))
    void panic(const char *, ...);

    [[noreturn]]
    void die(bool, const char *, va_list);
}

#endif //LIBSBOX_DIE_H
