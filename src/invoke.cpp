/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include <json.hpp>
#include <libsbox.h>
#include <iostream>
#include <fstream>

using json = nlohmann::json;

void error(const char *msg) {
    std::cout << json::object({{"error", msg}});
}

void error(const std::string &msg) {
    std::cout << json::object({{"error", msg}});
}

int main(int argc, char *argv[]) {
    libsbox::init(error);

    json j;
    try {
        if (argc == 1) {
            std::cin >> j;
        } else {
            std::ifstream in(argv[1]);
            if (!in.is_open()) {
                error(std::string() + "Cannot open file " + argv[1]);
            }
            in >> j;
        }
    } catch (json::parse_error &e) {
        error(e.what());
    }

    try {
        // here goes parsing
    } catch (json::type_error &e) {
        error(e.what());
    }
}
