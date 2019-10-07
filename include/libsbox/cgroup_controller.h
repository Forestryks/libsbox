/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_CGROUP_CONTROLLER_H_
#define LIBSBOX_CGROUP_CONTROLLER_H_

#include <string>
#include <filesystem>

namespace fs = std::filesystem;

class CgroupController {
public:
    explicit CgroupController(const std::string &name, const std::string &id);
    ~CgroupController();

    void die();
    void write(const std::string &filename, const std::string &data);
    std::string read(const std::string &filename);
    void enter();
private:
    fs::path path_;
};

#endif //LIBSBOX_CGROUP_CONTROLLER_H_
