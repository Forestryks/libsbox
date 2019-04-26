/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_EXECUTION_TARGET_H
#define LIBSBOX_EXECUTION_TARGET_H

#include <libsbox/io.h>
#include <libsbox/bind.h>

#include <vector>
#include <string>
#include <map>

namespace libsbox {
    class execution_target;
} // namespace libsbox

class libsbox::execution_target {
public:
    long time_limit = -1;
    long memory_limit = -1;
    long fsize_limit = -1;
    int max_files = 64;
    int max_threads = 1;
    long root_tmpfs_max_size = 1024 * 1024;

    std::string chdir_to = "/";

    char **argv;

    in_stream stdin;
    out_stream stdout, stderr;

    std::map<std::string, bind_rule> bind_rules;

    explicit execution_target(const std::vector<std::string> &);

    explicit execution_target(int, const char **);

    void init();

    ~execution_target() = default;
};

#endif //LIBSBOX_EXECUTION_TARGET_H
