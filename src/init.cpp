/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include <libsbox/init.h>
#include <libsbox/die.h>
#include <libsbox/signal.h>
#include <libsbox/logger.h>

#include <errno.h>
#include <cstring>
#include <unistd.h>
#include <sys/stat.h>
#include <exception>

bool libsbox::initialized = false;

void libsbox::init() {
    if (initialized) die("Already initialized");

    std::set_terminate([](){
        die("Uncaught exception");
    });

    init_credentials();
    prepare_signals();

    initialized = true;
}

void libsbox::init_credentials() {
    if (geteuid() != 0) die("Must be started as root");

    // May be unnecessary
    if (setresuid(0, 0, 0)) die("Cannot change to root user (%s)", strerror(errno));
    if (setresgid(0, 0, 0)) die("Cannot change to root group (%s)", strerror(errno));

    umask(0);
}
