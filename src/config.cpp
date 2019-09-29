/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include <libsbox/config.h>
#include <libsbox/daemon.h>
#include <libsbox/utils.h>

#include <json.hpp>
#include <fstream>
#include <iostream>

bool Config::loaded_ = false;
Config Config::config_;
const fs::path Config::path_ = "/etc/libsboxd/conf.json";

const Config &Config::get() {
    if (loaded_) return config_;
    config_.load();
    loaded_ = true;
    return config_;
}

void Config::load() {
    std::ifstream in(path_);
    if (!in.is_open()) {
        Daemon::get().die(format("Cannot open config file %s", path_.string().c_str()));
    }
    nlohmann::json json_object;
    try {
        in >> json_object;
    } catch (const nlohmann::json::parse_error &e) {
        Daemon::get().die(format("Failed to parse config file (%s)", e.what()));
    }

    try {
        num_boxes_ = json_object["num_boxes"];
        socket_path_ = json_object["socket_path"];
    } catch (const nlohmann::json::type_error &e) {
        Daemon::get().die(format("Failed to parse config file (%s)", e.what()));
    }
}

int Config::get_num_boxes() const {
    return num_boxes_;
}

const std::string &Config::get_socket_path() const {
    return socket_path_;
}
