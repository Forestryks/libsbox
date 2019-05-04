/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include <libsbox/cgroup.h>
#include <libsbox/fs.h>
#include <libsbox/die.h>

#include <iostream>
#include <unistd.h>
#include <cstring>

std::string libsbox::cgroup_controller::base_path = "/sys/fs/cgroup";

libsbox::cgroup_controller::cgroup_controller(const std::string &name) {
    std::string prefix = join_path(base_path, name);
    this->path = join_path(prefix, make_temp_dir(prefix));
    std::cout << this->path << std::endl;
}

libsbox::cgroup_controller::~cgroup_controller() {
    if (rmdir(this->path.c_str()) != 0) {
        libsbox::die("Cannot remove %s (%s)", this->path.c_str(), strerror(errno));
    }
}

void libsbox::cgroup_controller::die() {
    rmdir(this->path.c_str());
}
