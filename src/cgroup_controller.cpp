/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include "cgroup_controller.h"
#include "config.h"
#include "context_manager.h"
#include "utils.h"

#include <unistd.h>

CgroupController::CgroupController(const std::string &name, const std::string &id) {
    path_ = Config::get().get_cgroup_root() / name / ("libsbox-" + id);
    std::error_code error;
    fs::create_directories(path_, error);
    if (error) {
        die(format("Cannot create dir '%s': %s", path_.c_str(), error.message().c_str()));
    }
}

CgroupController::~CgroupController() {
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
    write("tasks", std::to_string(getpid()));
}
