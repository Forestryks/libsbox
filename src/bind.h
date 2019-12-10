/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_BIND_H_
#define LIBSBOX_BIND_H_

#include "task_data.h"

#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

class Bind {
public:
    enum Rules {
        RW = 1,
        DEV = 2,
        NOEXEC = 4,
        SUID = 8,
        OPT = 16
    };

    Bind(fs::path inside, fs::path outside, int flags);
    explicit Bind(const BindData *bind_data);

    void mount(const fs::path &root_dir, const fs::path &work_dir);
    void umount_if_mounted();

static void apply_standard_rules(const fs::path &root_dir, const fs::path &work_dir);
private:
    fs::path inside_;
    fs::path outside_;
    int flags_;
    bool mounted_ = false;

    fs::path from_;
    fs::path to_;

    void set_paths(const fs::path &root_dir, const fs::path &work_dir);

    static std::vector<Bind> standard_binds;
};

#endif //LIBSBOX_BIND_H_
