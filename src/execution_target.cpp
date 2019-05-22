/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include <libsbox/execution_target.h>
#include <libsbox/execution_context.h>
#include <libsbox/init.h>
#include <libsbox/die.h>

#include <cstring>
#include <iostream>
#include <unistd.h>

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

    this->bind_rules["lib"] = {"lib"};
    this->bind_rules["lib64"] = {"lib64", BIND_OPTIONAL};
    this->bind_rules["usr/lib"] = {"usr/lib"};
    this->bind_rules["usr/lib64"] = {"usr/lib64", BIND_OPTIONAL};

//    bind_rules["bin"] = {"bin"};
//    bind_rules["usr/bin"] = {"usr/bin"};
//    bind_rules["usr"] = {"usr"};
//    bind_rules["dev"] = {"dev", BIND_ALLOWDEV};
}

void libsbox::execution_target::die() {
    if (this->cpuacct_controller != nullptr) this->cpuacct_controller->die();
    if (this->memory_controller != nullptr) this->memory_controller->die();
    // remove cgroup
    // remove dir
}

void libsbox::execution_target::prepare() {
    this->cpuacct_controller = new cgroup_controller("cpuacct");
    this->memory_controller = new cgroup_controller("memory");
    // create cgroup
    // prepare cgroup
    // create dir
    // prepare dir
    // apply bind_rules
}

void libsbox::execution_target::cleanup() {
    delete this->cpuacct_controller;
    delete this->memory_controller;

    this->cpuacct_controller = nullptr;
    this->memory_controller = nullptr;
}

int libsbox::execution_target::proxy() {
    pid_t pid = getpid();
    std::cout << "My pid: " << pid << std::endl;
    exit(-1);
}

namespace libsbox {
    execution_target *current_target = nullptr;
} // namespace libsbox
