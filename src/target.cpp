/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include <libsbox/target.h>

#include <utility>
#include <iostream>

long Target::get_time_limit_ms() const {
    return time_limit_ms_;
}

long Target::get_wall_time_limit_ms() const {
    return wall_time_limit_ms_;
}

long Target::get_memory_limit_kb() const {
    return memory_limit_kb_;
}

long Target::get_fsize_limit_kb() const {
    return fsize_limit_kb_;
}

int Target::get_max_files() const {
    return max_files_;
}

int Target::get_max_threads() const {
    return max_threads_;
}

const std::string &Target::get_stdin() const {
    return stdin_;
}

const std::string &Target::get_stdout() const {
    return stdout_;
}

const std::string &Target::get_stderr() const {
    return stderr_;
}

const std::vector<Bind> &Target::get_binds() const {
    return binds_;
}

const std::vector<std::string> &Target::get_argv() const {
    return argv_;
}

// throws nlohmann::json::type_error
Target::Target(const nlohmann::json &json_object) {
    time_limit_ms_ = json_object["time_limit_ms"];
    wall_time_limit_ms_ = json_object["wall_time_limit_ms"];
    memory_limit_kb_ = json_object["memory_limit_kb"];
    fsize_limit_kb_ = json_object["fsize_limit_kb"];
    max_files_ = json_object["max_files"];
    max_threads_ = json_object["max_threads"];
    if (json_object["stdin"].is_null() || json_object["stdin"].empty()) {
        stdin_ = "-";
    } else {
        stdin_ = json_object["stdin"];
    }
    if (json_object["stdout"].is_null() || json_object["stdout"].empty()) {
        stdout_ = "-";
    } else {
        stdout_ = json_object["stdout"];
    }
    if (json_object["stderr"].is_null() || json_object["stderr"].empty()) {
        stderr_ = "-";
    } else {
        stderr_ = json_object["stderr"];
    }
    for (const auto &bind : json_object["binds"]) {
        binds_.emplace_back(bind["inside"], bind["outside"], bind["flags"]);
    }
    for (const auto &arg : json_object["argv"]) {
        argv_.push_back(arg);
    }
}
