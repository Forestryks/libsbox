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
    std::string path;

    void die();

    explicit cgroup_controller(const std::string &);
    ~cgroup_controller();

    void write(std::string, const std::string &);
    std::string read(std::string);
    void enter();
};

#endif //LIBSBOX_CGROUP_H
