/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include "config.h"
#include "context_manager.h"
#include "utils.h"

#include <rapidjson/istreamwrapper.h>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <fstream>

bool Config::loaded_ = false;
Config Config::config_;
fs::path Config::path_ = "/etc/libsboxd/conf.json";

const Config &Config::get() {
    if (loaded_) return config_;
    config_.load();
    loaded_ = true;
    return config_;
}

#define ERR() die("Config JSON is incorrect")
#define CHECK_MEMBER(obj, key) do { if (!obj.HasMember(key)) ERR(); } while (0)
#define CHECK_TYPE(obj, type) do { if (!obj.Is##type()) ERR(); } while (0)
#define GET(to, obj, type) do { CHECK_TYPE(obj, type); to = obj.Get##type(); } while (0)
#define GET_MEMBER(to, obj, key, type) do { CHECK_MEMBER(obj, key); GET(to, obj[key], type); } while (0)

void Config::load() {
    std::ifstream in(path_);
    if (!in.is_open()) {
        die(format("Cannot open config file %s", path_.string().c_str()));
    }

    rapidjson::IStreamWrapper i_stream_wrapper(in);
    rapidjson::Document document;
    document.ParseStream(i_stream_wrapper);

    if (document.HasParseError()) {
        die(format(
            "Failed to parse JSON: %s (at %zi)",
            GetParseError_En(document.GetParseError()),
            document.GetErrorOffset()
        ));
    }

    GET_MEMBER(num_boxes_, document, "num_boxes", Uint);
    GET_MEMBER(socket_path_, document, "socket_path", String);
    GET_MEMBER(first_uid_, document, "first_uid", Uint);
    GET_MEMBER(box_dir_, document, "box_dir", String);
    GET_MEMBER(cgroup_root_, document, "cgroup_root", String);
    GET_MEMBER(timer_interval_ms_, document, "timer_interval_ms", Int64);
}

#undef ERR
#undef CHECK_MEMBER
#undef CHECK_TYPE
#undef GET
#undef GET_MEMBER

uint32_t Config::get_num_boxes() const {
    return num_boxes_;
}

const fs::path &Config::get_socket_path() const {
    return socket_path_;
}

uid_t Config::get_first_uid() const {
    return first_uid_;
}

const fs::path &Config::get_box_dir() const {
    return box_dir_;
}

const fs::path &Config::get_cgroup_root() const {
    return cgroup_root_;
}

uint32_t Config::get_timer_interval_ms() const {
    return timer_interval_ms_;
}

void Config::set_path(const fs::path &path) {
    path_ = path;
}
