/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_EXECUTION_CONTEXT_H
#define LIBSBOX_EXECUTION_CONTEXT_H

#include <libsbox/io.h>
#include <libsbox/execution_target.h>
#include <libsbox/cgroup.h>

#include <vector>

namespace libsbox {
    class execution_context;

    extern execution_context *current_context;
} // namespace libsbox

class libsbox::execution_context {
public:
    long wall_time_limit = -1;
    uid_t first_uid = 35617; // [first_uid, first_uid+10)

    void register_target(execution_target *);
    void link(out_stream *, in_stream *, int pipe_flags = 0);
    void run();
private:
    int error_pipe[2];
    std::vector<execution_target *> targets;
    std::vector<io_pipe> pipes;
    struct timeval run_start = {};

    void create_pipes();
    void destroy_pipes();
    void die();

    void reset_wall_clock();
    long get_wall_clock();

    friend void die(const char *msg, ...);
    friend class execution_target;
};

#endif //LIBSBOX_EXECUTION_CONTEXT_H
