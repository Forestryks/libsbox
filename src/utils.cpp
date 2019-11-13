/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include "utils.h"
#include "context_manager.h"
#include "libsbox_internal.h"

#include <vector>
#include <cstring>
#include <cstdarg>
#include <string>
#include <unistd.h>
#include <fcntl.h>

std::string vformat(const char *fmt, va_list args) {
    std::vector<char> result(strlen(fmt) * 2);
    while (true) {
        va_list args2;
        va_copy(args2, args);
        int res = vsnprintf(result.data(), result.size(), fmt, args2);

        if ((res >= 0) && (static_cast<size_t>(res) < result.size())) {
            va_end(args);
            va_end(args2);
            return std::string(result.data());
        }

        size_t size;
        if (res < 0) {
            size = result.size() * 2;
        } else {
            size = static_cast<size_t>(res) + 1;
        }

        result.clear();
        result.resize(size);
        va_end(args2);
    }
}

std::string format(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    return vformat(fmt, args);
}

void write_file(const fs::path &path, const std::string &data) {
    fd_t fd = open(path.c_str(), O_WRONLY);
    if (fd < 0) {
        die(format("Cannot open file '%s' for writing: %m", path.c_str()));
    }
    int cnt = write(fd, data.c_str(), data.size());
    if (cnt < 0 || static_cast<size_t>(cnt) != data.size()) {
        die(format("Cannot write to file '%s': %m", path.c_str()));
    }
    if (close(fd) != 0) {
        die(format("Cannot close file '%s': %m", path.c_str()));
    }
}

namespace {
const size_t READ_BUF_SIZE = 2048;
char read_buf[READ_BUF_SIZE];
}

std::string read_file(const fs::path &path) {
    fd_t fd = open(path.c_str(), O_RDONLY);
    if (fd < 0) {
        die(format("Cannot open file '%s' for reading: %m", path.c_str()));
    }

    std::string res;
    while (true) {
        int cnt = read(fd, read_buf, READ_BUF_SIZE - 1);
        if (cnt < 0) {
            die(format("Cannot read from file '%s': %m", path.c_str()));
        }
        if (cnt == 0) break;
        read_buf[cnt] = 0;
        res += read_buf;
    }

    if (close(fd) != 0) {
        die(format("Cannot close file '%s': %m", path.c_str()));
    }
    return res;
}
