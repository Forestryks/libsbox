/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_TASK_DATA_H_
#define LIBSBOX_TASK_DATA_H_

// We must include linux header first to overwrite constants in libsbox header
#include <linux/limits.h>
#include <libsbox/limits.h>

struct IOStream {
    int fd = -1;
    char filename[PATH_MAX] = {};
};

struct BindData {
    char inside[PATH_MAX] = {};
    char outside[PATH_MAX] = {};
    int flags = 0;
};

struct TaskData {
    // parameters
    long time_limit_ms = -1;
    long wall_time_limit_ms = -1;
    long memory_limit_kb = -1;
    long fsize_limit_kb = -1;
    int max_files = 16;
    int max_threads = 1;

    IOStream stdin_desc, stdout_desc, stderr_desc;

    char *argv[ARGC_MAX] = {};
    char argv_data[ARGV_MAX] = {};
    char *env[ENVC_MAX] = {};
    char env_data[ENV_MAX] = {};

    int bind_count = -1;
    BindData binds[BINDS_MAX];

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

#endif //LIBSBOX_TASK_DATA_H_
