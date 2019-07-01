/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include <libsbox/logger.h>
#include <ctime>
#include <memory.h>
#include <cstdlib>
#include <cstdio>

const char *get_time_stamp() {
    time_t     now = time(0);
    tm  timestruct = {};
    char *buf = new char[80];
    timestruct = *localtime(&now);
    strftime(buf, 80, "%Y-%m-%d %X", &timestruct);
    return buf;
}

void put_time_stamp(const char *time_stamp) {
    delete[] time_stamp;
}

const char *levels[] = {
    "INFO ",
    "WARN ",
    "ERROR"
};

void internal_log(int level, const char *message, const char *file, const char *function, int line) {
    const char *time_stamp = get_time_stamp();

    fprintf(stderr, "[%s] [%s] at %s() (%s:%d): %s\n", time_stamp, levels[level], function, file, line, message);

    put_time_stamp(time_stamp);
}
void internal_log(int level, const std::string &message, const char *file, const char *function, int line) {
    internal_log(level, message.c_str(), file, function, line);
}
