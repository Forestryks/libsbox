/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_CONFIG_H
#define LIBSBOX_CONFIG_H

#include <filesystem>

namespace fs = std::filesystem;

class Config {
public:
    static const Config &get();

    [[nodiscard]] int get_num_boxes() const;
    [[nodiscard]] const fs::path & get_socket_path() const;
    [[nodiscard]] int get_first_uid() const;
    [[nodiscard]] const fs::path &get_box_dir() const;
    [[nodiscard]] const fs::path &get_cgroup_root() const;
    [[nodiscard]] int get_timer_interval_ms() const;
private:
    static Config config_;
    void load();
    static bool loaded_;
    const static fs::path path_;

    int num_boxes_;
    fs::path socket_path_;
    int first_uid_;
    fs::path box_dir_;
    fs::path cgroup_root_;
    int timer_interval_ms_;
};

#endif //LIBSBOX_CONFIG_H
