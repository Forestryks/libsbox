/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include <libsbox/utils.h>

#include <vector>
#include <cstring>

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
