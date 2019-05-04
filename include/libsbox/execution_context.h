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
    std::vector<execution_target *> targets;

    std::vector<io_pipe> pipes;
    std::vector<io_infile> input_files;
    std::vector<io_outfile> output_files;
public:
    void register_target(execution_target *);

    void link(out_stream *, in_stream *, int pipe_flags = 0);

    void link(const std::string &, in_stream *, int open_flags = 0);

    void link(out_stream *, const std::string &, int open_flags = 0);

    void run();

    void die();
};

#endif //LIBSBOX_EXECUTION_CONTEXT_H
