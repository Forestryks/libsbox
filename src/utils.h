/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_UTILS_H_
#define LIBSBOX_UTILS_H_

#include <string>
#include <filesystem>

namespace fs = std::filesystem;

// Format string using printf format from va_list
std::string vformat(const char *fmt, va_list args);

// Format string using printf format
__attribute__((format(printf, 1, 2)))
std::string format(const char *fmt, ...);

// Write data to file with error checks
void write_file(const fs::path &path, const std::string &data);

// Read whole file specified by path with error checks
std::string read_file(const fs::path &path);

#endif //LIBSBOX_UTILS_H_
