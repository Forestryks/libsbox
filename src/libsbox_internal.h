/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_DEFINES_H_
#define LIBSBOX_DEFINES_H_

#include "libsbox.h"

#include <stdint.h>

static const size_t ARGC_MAX = libsbox::ARGC_MAX;
static const size_t ENVC_MAX = libsbox::ENVC_MAX;
static const size_t BINDS_MAX = libsbox::BINDS_MAX;
static const size_t ARGV_MAX = libsbox::ARGV_MAX;
static const size_t ENV_MAX = libsbox::ENV_MAX;

using time_ms_t = libsbox::time_ms_t;
using memory_kb_t = libsbox::memory_kb_t;
using fd_t = libsbox::fd_t;

static_assert(sizeof(int) == sizeof(int32_t));

#endif //LIBSBOX_LIMITS_H_
