/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include "cgroup_controller.h"
#include "config.h"
#include "context_manager.h"
#include "utils.h"

#include <unistd.h>
#include <fcntl.h>

CgroupController::CgroupController(const std::string &name, const std::string &id) {
    path_ = Config::get().get_cgroup_root() / name / "libsbox" / id;
    std::error_code error;
    fs::create_directories(path_, error);
    if (error) {
        die(format("Cannot create dir '%s': %s", path_.c_str(), error.message().c_str()));
    }
}

CgroupController::~CgroupController() {
    if (enter_fd_ != -1) close_enter_fd();
    std::error_code error;
    fs::remove(path_, error);
    if (error) {
        die(format("Cannot remove dir '%s': %s", path_.c_str(), error.message().c_str()));
    }
}

void CgroupController::_die() {
    std::error_code error;
    fs::remove(path_, error);
}

void CgroupController::write(const std::string &filename, const std::string &data) {
    write_file(path_ / filename, data);
}

std::string CgroupController::read(const std::string &filename) {
    return read_file(path_ / filename);
}

void CgroupController::enter() {
    if (enter_fd_ == -1) {
        die("Cgroup enter was not delayed");
    }
    std::string to_write = std::to_string(getpid());
    int cnt = ::write(enter_fd_, to_write.c_str(), to_write.size());
    if (cnt < 0 || static_cast<size_t>(cnt) != to_write.size()) {
        die(format("Cannot write to cgroups tasks file: %m"));
    }
    close_enter_fd();
}

void CgroupController::init(const std::string &name) {
    fs::path path = Config::get().get_cgroup_root() / name / "libsbox";
    std::error_code error;
    fs::remove(path, error);
    if (error) {
        die(format("Cannot remove dir '%s': %s", path.c_str(), error.message().c_str()));
    }
    fs::create_directories(path, error);
    if (error) {
        die(format("Cannot create dir '%s': %s", path.c_str(), error.message().c_str()));
    }
}

void CgroupController::delay_enter() {
    if (enter_fd_ != -1) {
        return;
    }
    fs::path path = path_ / "tasks";
    enter_fd_ = open(path.c_str(), O_WRONLY | O_CLOEXEC);
    if (enter_fd_ < 0) {
        die(format("Cannot open file '%s' for writing: %m", path.c_str()));
    }
}

fd_t CgroupController::get_enter_fd() {
    return enter_fd_;
}

void CgroupController::close_enter_fd() {
    if (close(enter_fd_) != 0) {
        die(format("Cannot close cgroups tasks file: %m"));
    }
    enter_fd_ = -1;
}
