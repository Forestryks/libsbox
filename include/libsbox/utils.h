/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_UTILS_H_
#define LIBSBOX_UTILS_H_

#include <string>
#include <cstdarg>

#include <filesystem>

namespace fs = std::filesystem;

std::string vformat(const char *fmt, va_list args);

__attribute__((format(printf, 1, 2)))
std::string format(const char *fmt, ...);

class StringFormatter {
public:
    explicit StringFormatter(std::string str) : str(std::move(str)) {}

    explicit StringFormatter(const char *ptr) : str(ptr) {}

    template<typename ...T>
    std::string operator()(T... args) {
        return format(str.c_str(), args...);
    }

private:
    std::string str;
};

StringFormatter operator "" _format(const char *s, std::size_t len);

void write_file(const fs::path &path, const std::string &data);
std::string read_file(const fs::path &path);

#endif //LIBSBOX_UTILS_H_
