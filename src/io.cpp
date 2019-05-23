/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include <libsbox/io.h>
#include <libsbox/die.h>

void libsbox::io_stream::freopen(const std::string &filename) {
    if (!this->filename.empty()) {
        die("Cannot open file for stream: stream busy");
    }
    this->filename = filename;
}

void libsbox::io_stream::reset() {
    this->filename.clear();
}
