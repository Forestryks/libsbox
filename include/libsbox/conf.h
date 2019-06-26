/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_CONF_H
#define LIBSBOX_CONF_H

#include <unistd.h>
#include <string>

namespace libsbox {
	const int max_targets = 10;
    // const int pipe_size = 10 * 1024 * 1024;
    const int clone_stack_size = 1024 * 1024; // KB
    const int err_buf_size = 4096; // bytes
    const int cg_buf_size = 256; // bytes
    const long root_tmpfs_size = 1024 * 1024; // MB
    const long timer_interval = 50; // ms
    const std::string cgroup_base_path = "/sys/fs/cgroup";
} // namespace libsbox

#endif //LIBSBOX_CONF_H
