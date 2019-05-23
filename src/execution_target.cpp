/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include <libsbox/execution_target.h>
#include <libsbox/execution_context.h>
#include <libsbox/init.h>
#include <libsbox/die.h>
#include <libsbox/conf.h>
#include <libsbox/fs.h>
#include <libsbox/signal.h>

#include <cstring>
#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <string>
#include <sys/mount.h>
#include <sys/stat.h>

libsbox::execution_target::execution_target(const std::vector<std::string> &argv) {
    if (argv.empty()) libsbox::die("argv length must be at least 1");
    this->argv = new char *[argv.size() + 1];
    for (int i = 0; i < argv.size(); ++i) {
        this->argv[i] = strdup(argv[i].c_str());
    }
    this->argv[argv.size()] = nullptr;
    this->init();
}

libsbox::execution_target::execution_target(int argc, const char **argv) {
    if (argc == 0) libsbox::die("argv length must be at least 1");
    this->argv = new char *[argc + 1];
    for (int i = 0; i < argc; ++i) {
        this->argv[i] = strdup(argv[i]);
    }
    this->argv[argc] = nullptr;
    this->init();
}

void libsbox::execution_target::init() {
    if (!initialized) libsbox::die("Cannot create execution_target while not initialized");

    this->bind_rules["/lib"] = {"/lib"};
    this->bind_rules["/lib64"] = {"/lib64", BIND_OPTIONAL};
    this->bind_rules["/usr/lib"] = {"/usr/lib"};
    this->bind_rules["/usr/lib64"] = {"/usr/lib64", BIND_OPTIONAL};

//    bind_rules["bin"] = {"bin"};
//    bind_rules["usr/bin"] = {"usr/bin"};
//    bind_rules["usr"] = {"usr"};
//    bind_rules["dev"] = {"dev", BIND_ALLOWDEV};
}

void libsbox::execution_target::die() {
    if (this->cpuacct_controller != nullptr) this->cpuacct_controller->die();
    if (this->memory_controller != nullptr) this->memory_controller->die();

    rmdir(this->id.c_str());
    // remove cgroup
    // remove dir
}

void libsbox::execution_target::prepare() {
    this->cpuacct_controller = new cgroup_controller("cpuacct");
    this->memory_controller = new cgroup_controller("memory");

    if (this->memory_limit != -1) {
        this->memory_controller->write("memory.limit_in_bytes", std::to_string(this->memory_limit) + "K");
    }

    this->id = make_temp_dir(".", 0750);
}

void libsbox::execution_target::cleanup() {
    delete this->cpuacct_controller;
    delete this->memory_controller;

    this->cpuacct_controller = nullptr;
    this->memory_controller = nullptr;

    this->proxy_pid = 0;
    this->target_pid = 0;

    if (rmdir(this->id.c_str()) != 0) {
        libsbox::die("Cannot remove dir %s (%s)", this->id.c_str(), strerror(errno));
    }
}

void libsbox::execution_target::prepare_root() {
    if (mount(nullptr, "/", nullptr, MS_REC | MS_PRIVATE, nullptr) < 0) {
        libsbox::die("Cannot privatize mounts (%s)", strerror(errno));
    }

    std::string param = "mode=755,size=" + std::to_string(root_tmpfs_size);
    if (mount("none", this->id.c_str(), "tmpfs", 0, param.c_str()) < 0) {
        libsbox::die("Cannot mount tmpfs (%s)", strerror(errno));
    }

    std::string proc_dir = join_path(this->id, "proc");
    make_path(proc_dir, 0777);
    if (mount("none", proc_dir.c_str(), "proc", 0, "") < 0) {
        libsbox::die("Cannot mount proc filesystem (%s)", strerror(errno));
    }
    if (mount("none", proc_dir.c_str(), "proc", MS_REMOUNT, "hidepid=2") < 0) {
        libsbox::die("Cannot remount proc filesystem (%s)", strerror(errno));
    }

    std::string work_dir = join_path(this->id, "work");
    make_path(work_dir, 0777);

    for (const auto &rule : this->bind_rules) {
        std::string inside, outside;
        if (rule.first[0] == '/') {
            inside = join_path(this->id, rule.first);
        } else {
            inside = join_path(work_dir, rule.first);
        }
        outside = rule.second.path;
        int flags = rule.second.flags;

        if (!path_exists(outside)) {
            if (flags & BIND_OPTIONAL) continue;
            libsbox::die("Bind %s -> %s failed: path not exists", outside.c_str(), rule.first.c_str());
        }

        if (dir_exists(outside)) {
            // working with dir
            if (flags & (BIND_COPY_IN | BIND_COPY_OUT)) {
                libsbox::die("BIND_COPY_* are not compatible with directories");
            }

            make_path(inside, 0777);

            unsigned long mount_flags = (MS_BIND | MS_NOSUID | MS_NOEXEC | MS_REC);
            if (!(flags & BIND_READWRITE)) {
                mount_flags |= MS_RDONLY;
            }

            if (mount(outside.c_str(), inside.c_str(), "none", mount_flags, "") < 0) {
                libsbox::die("Bind %s -> %s failed: mount failed (%s)", outside.c_str(), rule.first.c_str(), strerror(errno));
            }
            if (mount(outside.c_str(), inside.c_str(), "none", mount_flags | MS_REMOUNT, "") < 0) {
                libsbox::die("Bind %s -> %s failed: remount failed (%s)", outside.c_str(), rule.first.c_str(), strerror(errno));
            }
        } else {
            // working with file
            if (flags & BIND_COPY_IN) {
                if (flags & BIND_READWRITE) copy_file(outside, inside, 0777);
                else copy_file(outside, inside, 0755);
                continue;
            }
            if (flags & BIND_COPY_OUT) continue;

            make_file(inside, 0777);

            unsigned long mount_flags = (MS_BIND | MS_NOSUID | MS_NOEXEC);
            if (!(flags & BIND_READWRITE)) {
                mount_flags |= MS_RDONLY;
            }

            if (mount(outside.c_str(), inside.c_str(), "none", mount_flags, "") < 0) {
                libsbox::die("Bind %s -> %s failed: mount failed (%s)", outside.c_str(), rule.first.c_str(), strerror(errno));
            }
            if (mount(outside.c_str(), inside.c_str(), "none", mount_flags | MS_REMOUNT, "") < 0) {
                libsbox::die("Bind %s -> %s failed: remount failed (%s)", outside.c_str(), rule.first.c_str(), strerror(errno));
            }
        }
    }
}

int libsbox::execution_target::proxy() {
    reset_signals();

    if (close(this->status_pipe[0]) != 0) {
        libsbox::die("Cannot close read end of status pipe (%s)", strerror(errno));
    }
    if (close(current_context->error_pipe[0]) != 0) {
        libsbox::die("Cannot close read end of error pipe (%s)", strerror(errno));
    }

    prepare_root();

    exit(0);
}

namespace libsbox {
    int clone_callback(void *);
} // namespace libsbox

int libsbox::clone_callback(void *arg) {
    return current_target->proxy();
}

void libsbox::execution_target::start_proxy() {
    current_target = this;

    char *clone_stack = new char[clone_stack_size];
    // CLONE_NEWIPC and CLONE_NEWNS are dramatically slow
    // I don't know why
    this->proxy_pid = clone(
            clone_callback,
            clone_stack + clone_stack_size,
            SIGCHLD | CLONE_NEWIPC | CLONE_NEWNET | CLONE_NEWNS | CLONE_NEWPID,
            nullptr
    );
    delete[] clone_stack;

    current_target = nullptr;
    if (this->proxy_pid < 0) {
        libsbox::die("Cannot create proxy: fork failed (%s)", strerror(errno));
    }

    if (close(this->status_pipe[1]) != 0) {
        libsbox::die("Cannot close write end of status pipe (%s)", strerror(errno));
    }
}

namespace libsbox {
    execution_target *current_target = nullptr;
} // namespace libsbox
