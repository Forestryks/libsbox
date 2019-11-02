/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_TASK_DATA_H
#define LIBSBOX_TASK_DATA_H

#include "limits.h"
#include "plain_string.h"
#include "plain_vector.h"
#include "plain_string_vector.h"

#include <linux/limits.h>

struct IOStream {
    int fd = -1;
    PlainString<PATH_MAX> filename;
};

struct BindData {
    PlainString<PATH_MAX> inside;
    PlainString<PATH_MAX> outside;
    int flags{};
};

#include <iostream>

// TODO: normal debug
#define db(x) std::cerr << #x " = " << x << std::endl

struct TaskData {
    void dbg() {
        std::cerr << "=== task_data ===" << std::endl;
        db(time_limit_ms);
        db(wall_time_limit_ms);
        db(memory_limit_kb);
        db(fsize_limit_kb);
        db(max_files);
        db(max_threads);
        db(ipc);
        db(stdin_desc.filename.c_str());
        db(stdin_desc.fd);
        db(stdout_desc.filename.c_str());
        db(stdout_desc.fd);
        db(stderr_desc.filename.c_str());
        db(stderr_desc.fd);
        db(argv.size());
        for (size_t i = 0; i < argv.size(); ++i) {
            db(argv[i]);
        }
        db(env.size());
        for (size_t i = 0; i < env.size(); ++i) {
            db(env[i]);
        }
        db(binds.size());
        for (size_t i = 0; i < binds.size(); ++i) {
            db(binds[i].inside.c_str());
            db(binds[i].outside.c_str());
            db(binds[i].flags);
        }
        db(time_usage_ms);
        db(time_usage_sys_ms);
        db(time_usage_sys_ms);
        db(wall_time_usage_ms);
        db(memory_usage_kb);
        db(time_limit_exceeded);
        db(wall_time_limit_exceeded);
        db(exited);
        db(exit_code);
        db(signaled);
        db(term_signal);
        db(oom_killed);
        db(error);
        std::cerr << "=================" << std::endl;
    }

    // parameters
    long time_limit_ms = -1;
    long wall_time_limit_ms = -1;
    long memory_limit_kb = -1;
    long fsize_limit_kb = -1;
    int max_files = 16;
    int max_threads = 1;
    bool ipc = false;

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

    volatile bool error = false;
};

#endif //LIBSBOX_TASK_DATA_H
