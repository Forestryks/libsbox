/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include <libsbox/bind.h>
#include <libsbox/context_manager.h>
#include <libsbox/utils.h>

#include <sys/mount.h>
#include <iostream>

Bind::Bind(fs::path inside, fs::path outside, int flags)
    : inside_(std::move(inside)), outside_(std::move(outside)), flags_(flags) {}

Bind::Bind(const BindData *bind_data) : inside_(bind_data->inside), outside_(bind_data->outside), flags_(bind_data->flags) {}

const fs::path &Bind::get_inside() const {
    return inside_;
}

const fs::path &Bind::get_outside() const {
    return outside_;
}

int Bind::get_flags() const {
    return flags_;
}

std::vector<Bind> Bind::standard_binds = {
    {"/lib", "/lib", 0},
    {"/lib64", "/lib64", Rules::OPT},
    {"/bin", "/bin", 0},
    {"/dev", "/dev", Rules::DEV},
    {"/usr", "/usr", 0} // MAY BE UNSAFE
};

std::pair<fs::path, fs::path> Bind::get_paths(const fs::path &root_dir, const fs::path &work_dir) {
    if (!outside_.is_absolute()) {
        ContextManager::get().die(format("%s is not absolute path", outside_.c_str()));
    }
    fs::path from = outside_;
    fs::path to;
    if (inside_.is_absolute()) {
        to = root_dir / inside_.relative_path();
    } else {
        to = work_dir / inside_;
    }
    return {from, to};
}

void Bind::mount(const fs::path &root_dir, const fs::path &work_dir) {
    auto [from, to] = get_paths(root_dir, work_dir);
    int mount_flags = (MS_BIND | MS_REC);
    if (!(flags_ & Rules::RW)) {
        mount_flags |= MS_RDONLY;
    }
    if (!(flags_ & Rules::DEV)) {
        mount_flags |= MS_NODEV;
    }
    if (flags_ & Rules::NOEXEC) {
        mount_flags |= MS_NOEXEC;
    }
    if (!(flags_ & Rules::SUID)) {
        mount_flags |= MS_NOSUID;
    }

    bool optional = (flags_ & Rules::OPT);

    std::error_code error;
    if (!fs::exists(from, error)) {
        if (optional) return;
        ContextManager::get().die(format("%s not exists", from.c_str()));
    }

    if (fs::is_directory(from, error)) {
        fs::create_directories(to, error);
        if (error) {
            if (optional) return;
            ContextManager::get().die(format("Cannot create dir %s: %s", to.c_str(), error.message().c_str()));
        }
        if (::mount(from.c_str(), to.c_str(), "none", mount_flags, "") < 0) {
            if (optional) return;
            ContextManager::get().die(format("Cannot mount %s: %m", to.c_str()));
        }
        mounted_ = true;
    } else {
        ContextManager::get().die(format("%s is not directory. Currently libsboxd supports only directories, sorry :(", to.c_str()));
    }
    if (error && !optional) {
        ContextManager::get().die(format("Cannot stat %s: %s", from.c_str(), error.message().c_str()));
    }
}

void Bind::umount(const fs::path &root_dir, const fs::path &work_dir) {
    if (!mounted_) return;
    auto [from, to] = get_paths(root_dir, work_dir);
    if (::umount(to.c_str()) != 0) {
        ContextManager::get().die(format("Cannot umount %s: %m", to.c_str()));
    }
    mounted_ = false;
}

std::vector<Bind> &Bind::get_standard_binds() {
    return standard_binds;
}
