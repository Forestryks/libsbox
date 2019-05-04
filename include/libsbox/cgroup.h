/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_CGROUP_H
#define LIBSBOX_CGROUP_H

#include <string>

namespace libsbox {
    class cgroup_controller;
} // namespace libsbox

class libsbox::cgroup_controller {
public:
    static std::string base_path;
    std::string path;

    void die();

    explicit cgroup_controller(const std::string &);
    ~cgroup_controller();
};

#endif //LIBSBOX_CGROUP_H
