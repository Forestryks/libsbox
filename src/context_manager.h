/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_CONTEXT_MANAGER_H_
#define LIBSBOX_CONTEXT_MANAGER_H_

#include <string>

class ContextManager {
public:
    // Get current context manager
    static ContextManager &get();
    // Set new context manager. Must be called as fast as possible after fork() or clone()
    static void set(ContextManager *context);

    // Called on critical error
    [[noreturn]] virtual void die(const std::string &error) = 0;
    // Called when receiving SIGTERM asynchronously (in signal handler)
    virtual void terminate() = 0;
private:
    static ContextManager *context_;
};

#endif //LIBSBOX_CONTEXT_MANAGER_H_
