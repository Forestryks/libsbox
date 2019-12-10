/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include "logger.h"
#include "context_manager.h"
#include "utils.h"

#include <unistd.h>
#include <fcntl.h>

Logger *Logger::logger_ = nullptr;

void Logger::init() {
    if (logger_ != nullptr) {
        die("Logger already initialized");
    }
    logger_ = new Logger();
}

Logger &Logger::get() {
    return *logger_;
}

fd_t Logger::get_fd() const {
    return fd_;
}

void Logger::_log(std::string msg) {
    msg = format("[%s] %s\n", ContextManager::get().get_name().c_str(), msg.c_str());
    // Will be atomic if msg.size() <= PIPE_BUF (4096)
    if (write(fd_, msg.c_str(), msg.size()) < 0) {
        die(format("Logger write() failed: %m"));
    }
}

Logger::Logger() {
    fd_ = dup3(STDERR_FILENO, 107, O_CLOEXEC);
    if (fd_ < 0) {
        die(format("Logger dup3() failed: %m"));
    }
}
