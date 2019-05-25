/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include <libsbox/cgroup.h>
#include <libsbox/fs.h>
#include <libsbox/die.h>
#include <libsbox/conf.h>

#include <unistd.h>
#include <cstring>
#include <fcntl.h>

std::string libsbox::cgroup_controller::base_path = "/sys/fs/cgroup";

libsbox::cgroup_controller::cgroup_controller(const std::string &name) {
    std::string prefix = join_path(base_path, name);
    this->path = join_path(prefix, make_temp_dir(prefix));
}

libsbox::cgroup_controller::~cgroup_controller() {
    if (rmdir(this->path.c_str()) != 0) {
        libsbox::die("Cannot remove %s (%s)", this->path.c_str(), strerror(errno));
    }
}

void libsbox::cgroup_controller::die() {
    rmdir(this->path.c_str());
}

void libsbox::cgroup_controller::write(std::string filename, const std::string &data) {
    filename = join_path(this->path, filename);
    int fd = open(filename.c_str(), O_WRONLY);
    if (fd < 0) {
        libsbox::die("Cg write failed: cannot open file %s (%s)", filename.c_str(), strerror(errno));
    }

    if (::write(fd, data.c_str(), data.size()) != (int)data.size()) {
        libsbox::die("Cg write failed: cannot write to %s (%s)", filename.c_str(), strerror(errno));
    }

    if (close(fd) != 0) {
        libsbox::die("Cg write failed: cannot close file %s (%s)", filename.c_str(), strerror(errno));
    }
}

std::string libsbox::cgroup_controller::read(std::string filename) {
    filename = join_path(this->path, filename);
    int fd = open(filename.c_str(), O_RDONLY);
    if (fd < 0) {
        libsbox::die("Cg read failed: cannot open file %s (%s)", filename.c_str(), strerror(errno));
    }

    char buf[cg_buf_size];
    int cnt = ::read(fd, buf, cg_buf_size);
    if (cnt < 0) {
        libsbox::die("Cg read failed: cannot read from %s (%s)", filename.c_str(), strerror(errno));
    }
    buf[cnt] = 0;

    if (close(fd) != 0) {
        libsbox::die("Cg read failed: cannot close file %s (%s)", filename.c_str(), strerror(errno));
    }

    return std::string(buf);
}

void libsbox::cgroup_controller::enter() {
    this->write("tasks", std::to_string(getpid()));
}
