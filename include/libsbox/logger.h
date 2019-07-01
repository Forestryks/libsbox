/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_LOGGER_H
#define LIBSBOX_LOGGER_H

#include <string>

#define _log(level, message) internal_log(level, message, (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__), __FUNCTION__, __LINE__)

#define INFO 0
#define WARN 1
#define ERROR 2

#define info(message) _log(INFO, message)
#define warn(message) _log(WARN, message)
#define error(message) _log(ERROR, message)

void internal_log(int level, const char *message, const char *file, const char *function, int line);
void internal_log(int level, const std::string &message, const char *file, const char *function, int line);

#endif //LIBSBOX_LOGGER_H
