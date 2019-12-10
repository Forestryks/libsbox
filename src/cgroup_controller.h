/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_CGROUP_CONTROLLER_H_
#define LIBSBOX_CGROUP_CONTROLLER_H_

#include "libsbox_internal.h"

#include <string>
#include <filesystem>

namespace fs = std::filesystem;

class CgroupController {
public:
    explicit CgroupController(const std::string &name, const std::string &id);
    ~CgroupController();

    static void init(const std::string &name);
    void _die();
    void write(const std::string &filename, const std::string &data);
    std::string read(const std::string &filename);
    void delay_enter();
    fd_t get_enter_fd();
    void enter();
private:
    fs::path path_;
    fd_t enter_fd_ = -1;
    void close_enter_fd();
};

#endif //LIBSBOX_CGROUP_CONTROLLER_H_
