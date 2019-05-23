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
    int error_pipe[2];

    std::vector<execution_target *> targets;

    std::vector<io_pipe> pipes;

    void create_pipes();

    void destroy_pipes();

    void register_target(execution_target *);

    void link(out_stream *, in_stream *, int pipe_flags = 0);

    void run();

    void die();

    struct timeval run_start = {};
    void reset_wall_clock();
    long get_wall_clock();
};

#endif //LIBSBOX_EXECUTION_CONTEXT_H
