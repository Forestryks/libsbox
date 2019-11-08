/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_DEFINES_H_
#define LIBSBOX_DEFINES_H_

#include <stdint.h>

#define ARGC_MAX 128
#define ENVC_MAX 0
#define BINDS_MAX 10
#define ARGV_MAX 4096
#define ENV_MAX 0

using fd_t = int;
using time_ms_t = int64_t;
using memory_kb_t = int64_t;
static_assert(sizeof(int) == sizeof(int32_t));

#endif //LIBSBOX_LIMITS_H_
