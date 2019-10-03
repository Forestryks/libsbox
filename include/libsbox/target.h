/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_TARGET_H_
#define LIBSBOX_TARGET_H_

#include <libsbox/bind.h>

#include <string>
#include <vector>
#include <json.hpp>

class Target {
public:
    explicit Target(const nlohmann::json &json_object);
    [[nodiscard]] long get_time_limit_ms() const;
    [[nodiscard]] long get_wall_time_limit_ms() const;
    [[nodiscard]] long get_memory_limit_kb() const;
    [[nodiscard]] long get_fsize_limit_kb() const;
    [[nodiscard]] int get_max_files() const;
    [[nodiscard]] int get_max_threads() const;
    [[nodiscard]] const std::string &get_stdin() const;
    [[nodiscard]] const std::string &get_stdout() const;
    [[nodiscard]] const std::string &get_stderr() const;
    [[nodiscard]] const std::vector<Bind> &get_binds() const;
    [[nodiscard]] const std::vector<std::string> &get_argv() const;
private:
    long time_limit_ms_;
    long wall_time_limit_ms_;
    long memory_limit_kb_;
    long fsize_limit_kb_;
    int max_files_;
    int max_threads_;
    std::string stdin_, stdout_, stderr_;
    std::vector<Bind> binds_;
    std::vector<std::string> argv_;
};

#endif //LIBSBOX_TARGET_H_
