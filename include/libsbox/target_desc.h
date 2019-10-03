/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_TARGET_DESC_H_
#define LIBSBOX_TARGET_DESC_H_

#include <linux/limits.h>

const int ARGC_MAX = 256;
const int ENVC_MAX = 256;
const int BINDS_MAX = 256;

struct io_stream {
    int fd = -1;
    char filename[PATH_MAX] = {};
};

struct bind_desc {
    char inside[PATH_MAX] = {};
    char outside[PATH_MAX] = {};
    int flags = 0;
};

struct target_desc {
    // parameters
    long time_limit_ms = -1;
    long wall_time_limit_ms = -1;
    long memory_limit_kb = -1;
    long fsize_limit_kb = -1;
    int max_files = 16;
    int max_threads = 1;

    io_stream stdin_desc, stdout_desc, stderr_desc;

    char *argv[ARGC_MAX] = {};
    char argv_data[ARG_MAX] = {};
    char *env[ENVC_MAX] = {};
    char env_data[ARG_MAX] = {};

    int bind_count = -1;
    bind_desc binds[BINDS_MAX];

    // results
    long time_usage_ms = 0;
    long time_usage_sys_ms = 0;
    long time_usage_user_ms = 0;
    long wall_time_usage_ms = 0;
    long memory_usage_kb = 0;

    bool time_limit_exceeded = false;
    bool wall_time_limit_exceeded = false;
    bool exited = false;
    int exit_code = -1;
    bool signaled = false;
    int term_signal = -1;
    bool oom_killed = false;

    bool error = false;
};

#endif //LIBSBOX_TARGET_DESC_H_
