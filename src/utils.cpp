/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include <libsbox/utils.h>

#include <vector>
#include <cstring>

#include <libsbox/context.h>

#include <fcntl.h>
#include <unistd.h>

std::string vformat(const char *fmt, va_list args) {
    std::vector<char> v(std::min(512, (int)strlen(fmt) * 2));
    while (true) {
        va_list args2;
        va_copy(args2, args);
        int res = vsnprintf(v.data(), v.size(), fmt, args2);

        if ((res >= 0) && (res < static_cast<int>(v.size()))) {
            va_end(args);
            va_end(args2);
            return std::string(v.data());
        }

        size_t size;
        if (res < 0) {
            size = v.size() * 2;
        } else {
            size = static_cast<size_t>(res) + 1;
        }

        v.clear();
        v.resize(size);
        va_end(args2);
    }
}

std::string format(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    return vformat(fmt, args);
}

StringFormatter operator ""_format(const char *s, [[maybe_unused]] std::size_t len) {
    return StringFormatter(s);
}

void write_file(const fs::path &path, const std::string &data) {
    int fd = open(path.c_str(), O_WRONLY);
    if (fd < 0) {
        Context::get().die(format("Cannot open file '%s' for writing: %m", path.c_str()));
    }
    if (write(fd, data.c_str(), data.size()) != (int)data.size()) {
        Context::get().die(format("Cannot write to file '%s': %m", path.c_str()));
    }
    if (close(fd) != 0) {
        Context::get().die(format("Cannot close file '%s': %m", path.c_str()));
    }
}

const int READ_BUF_SIZE = 2048;
char read_buf[READ_BUF_SIZE];

std::string read_file(const fs::path &path) {
    int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0) {
        Context::get().die(format("Cannot open file '%s' for reading: %m", path.c_str()));
    }

    std::string res;
    while (true) {
        int cnt = read(fd, read_buf, READ_BUF_SIZE - 1);
        if (cnt < 0) {
            Context::get().die(format("Cannot read from file '%s': %m", path.c_str()));
        }
        if (cnt == 0) break;
        read_buf[cnt] = 0;
        res += read_buf;
    }

    if (close(fd) != 0) {
        Context::get().die(format("Cannot close file '%s': %m", path.c_str()));
    }
    return res;
}
