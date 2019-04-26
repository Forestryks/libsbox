/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include <libsbox/init.h>
#include <libsbox/die.h>

#include <errno.h>
#include <cstring>
#include <unistd.h>
#include <sys/stat.h>

bool libsbox::initialized = false;

void libsbox::init(void (*fatal_error_handler)(const char *)) {
    fatal_handler = fatal_error_handler;
    if (initialized) die("Already initialized");

    init_credentials();

    initialized = true;
}

void libsbox::init_credentials() {
    if (geteuid() != 0) die("Must be started as root");

    // May be unnecessary
    if (setresuid(0, 0, 0)) die("Cannot change to root user (%s)", strerror(errno));
    if (setresgid(0, 0, 0)) die("Cannot change to root group (%s)", strerror(errno));

    umask(022);
}
