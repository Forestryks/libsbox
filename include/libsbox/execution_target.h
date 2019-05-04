/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_EXECUTION_TARGET_H
#define LIBSBOX_EXECUTION_TARGET_H

#include <libsbox/io.h>
#include <libsbox/bind.h>
#include <libsbox/cgroup.h>

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

    uid_t uid = 31313;
    gid_t gid = 31313;

    cgroup_controller *cpuacct_controller, *memory_controller;

    explicit execution_target(const std::vector<std::string> &);

    execution_target(int, const char **);

    ~execution_target() = default;

    void init();

    void die();

    void prepare();

    void cleanup();
};

#endif //LIBSBOX_EXECUTION_TARGET_H
