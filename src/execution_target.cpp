/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include <libsbox/execution_target.h>

#include <cstring>
#include <iostream>

libsbox::execution_target::execution_target(const std::vector<std::string> &argv) {
    this->argv = new char *[argv.size() + 1];
    for (int i = 0; i < argv.size(); ++i) {
        this->argv[i] = strdup(argv[i].c_str());
    }
    this->argv[argv.size()] = nullptr;
    this->init();
}

libsbox::execution_target::execution_target(int argc, const char **argv) {
    this->argv = new char *[argc + 1];
    for (int i = 0; i < argc; ++i) {
        this->argv[i] = strdup(argv[i]);
    }
    this->argv[argc] = nullptr;
    this->init();
}

void libsbox::execution_target::init() {
    bind_rules["bin"] = {"bin"};
    bind_rules["lib"] = {"lib"};
    bind_rules["lib64"] = {"lib64", BIND_OPTIONAL};
    bind_rules["usr"] = {"usr"};
    bind_rules["dev"] = {"dev", BIND_ALLOWDEV};
}
