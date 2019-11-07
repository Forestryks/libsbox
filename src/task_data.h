/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_TASK_DATA_H
#define LIBSBOX_TASK_DATA_H

#include "plain_string.h"
#include "plain_vector.h"
#include "plain_string_vector.h"
#include "limits.h"
#include "context_manager.h"
#include "utils.h"

#include <limits.h>

struct IOStream {
    int fd = -1;
    PlainString<PATH_MAX> filename;
};

struct BindData {
    BindData(const std::string &inside, const std::string &outside, int flags) {
        if (inside.size() > inside_.max_size()) {
            die(format("bind inside path is larger that maximum (%zi > %zi)", inside.size(), inside_.max_size()));
        }
        if (outside.size() > outside_.max_size()) {
            die(format("bind outsize path is larger that maximum (%zi > %zi)", outside.size(), outside_.max_size()));
        }
        inside_ = inside;
        outside_ = outside;
        flags_ = flags;
    };
    PlainString<PATH_MAX> inside_;
    PlainString<PATH_MAX> outside_;
    int flags_;
};

struct TaskData {
    // parameters
    long time_limit_ms = -1;
    long wall_time_limit_ms = -1;
    long memory_limit_kb = -1;
    long fsize_limit_kb = -1;
    int max_files = 16;
    int max_threads = 1;
    bool ipc = false;
    bool standard_binds = true;

    IOStream stdin_desc, stdout_desc, stderr_desc;
    PlainStringVector<ARGC_MAX, ARGV_MAX> argv;
    PlainStringVector<ENVC_MAX, ENV_MAX> env;

    PlainVector<BindData, BINDS_MAX> binds;

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
    bool memory_limit_hit = false;

    volatile bool error = false;
};

#endif //LIBSBOX_TASK_DATA_H
