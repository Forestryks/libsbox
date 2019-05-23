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

    extern execution_target *current_target;
} // namespace libsbox

class libsbox::execution_target {
public:
    long time_limit = -1;
    long memory_limit = -1;
    long fsize_limit = -1;
    int max_files = 64;
    int max_threads = 1;

    char **argv;

    in_stream stdin;
    out_stream stdout, stderr;

    std::map<std::string, bind_rule> bind_rules;

    cgroup_controller *cpuacct_controller = nullptr, *memory_controller = nullptr;

    pid_t proxy_pid = 0;
    pid_t target_pid = 0;

    int status_pipe[2] = {-1, -1};

    std::string id;

    bool inside_box = false;

    explicit execution_target(const std::vector<std::string> &);

    execution_target(int, const char **);

    ~execution_target() = default;

    void init();

    void die();

    void prepare();

    void cleanup();

    int proxy();

    void start_proxy();

    void prepare_root();
};

#endif //LIBSBOX_EXECUTION_TARGET_H
