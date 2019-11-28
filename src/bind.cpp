/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include "bind.h"
#include "context_manager.h"
#include "utils.h"

#include <sys/mount.h>
#include <fcntl.h>

Bind::Bind(fs::path inside, fs::path outside, int flags)
    : inside_(std::move(inside)), outside_(std::move(outside)), flags_(flags) {}

Bind::Bind(const BindData *bind_data)
    : inside_(bind_data->inside_.c_str()), outside_(bind_data->outside_.c_str()), flags_(bind_data->flags_) {}

std::vector<Bind> Bind::standard_binds = {
    {"/lib", "/lib", 0},
    {"/lib64", "/lib64", Rules::OPT},
    {"/bin", "/bin", 0},
    {"/dev", "/dev", Rules::DEV},
    {"/usr", "/usr", 0} // MAY BE UNSAFE
};

void Bind::set_paths(const fs::path &root_dir, const fs::path &work_dir) {
    if (!outside_.is_absolute()) {
        die(format("%s is not absolute path", outside_.c_str()));
    }
    from_ = outside_;
    if (inside_.is_absolute()) {
        to_ = root_dir / inside_.relative_path();
    } else {
        to_ = work_dir / inside_;
    }
}

void Bind::mount(const fs::path &root_dir, const fs::path &work_dir) {
    set_paths(root_dir, work_dir);
    unsigned long mount_flags = (MS_BIND | MS_REC);
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
    if (!fs::exists(from_, error)) {
        if (optional) return;
        die(format("%s not exists", from_.c_str()));
    }

    if (fs::is_directory(from_, error)) {
        fs::create_directories(to_, error);
        if (error) {
            die(format("Cannot create dir %s for mount point: %s", to_.c_str(), error.message().c_str()));
        }
        if (::mount(from_.c_str(), to_.c_str(), "none", mount_flags, "") < 0) {
            die(format("Cannot mount %s: %m", to_.c_str()));
        }
        mounted_ = true;
    } else if (fs::is_regular_file(from_, error)) {
        fs::create_directories(to_.parent_path(), error);
        if (error) {
            die(format("Cannot create dir %s: %s", from_.parent_path().c_str(), error.message().c_str()));
        }
        int fd = open(to_.c_str(), O_CREAT | O_WRONLY | O_TRUNC);
        if (fd < 0) {
            die(format("Cannot create file %s for mount point: %m", to_.c_str()));
        }
        if (close(fd) < 0) {
            die(format("Cannot close file desctiptor: %m"));
        }
        if (::mount(from_.c_str(), to_.c_str(), "none", mount_flags, "") < 0) {
            die(format("Cannot mount %s: %m", to_.c_str()));
        }
        mounted_ = true;
    } else {
        die(format("%s is not directory nor regular file", to_.c_str()));
    }
    if (error) {
        die(format("Cannot stat %s: %s", from_.c_str(), error.message().c_str()));
    }
}

void Bind::umount_if_mounted() {
    if (!mounted_) return;
    if (::umount(to_.c_str()) != 0) {
        die(format("Cannot umount %s: %m", to_.c_str()));
    }
    mounted_ = false;
}

void Bind::apply_standard_rules(const fs::path &root_dir, const fs::path &work_dir) {
    for (auto &bind : standard_binds) {
        bind.mount(root_dir, work_dir);
    }
}
