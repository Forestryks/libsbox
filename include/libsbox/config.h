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
    [[nodiscard]] const std::string &get_socket_path() const;
private:
    static Config config_;
    void load();
    static bool loaded_;
    const static fs::path path_;

    int num_boxes_;
    std::string socket_path_;
};

#endif //LIBSBOX_CONFIG_H
