/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_CONTEXT_MANAGER_H_
#define LIBSBOX_CONTEXT_MANAGER_H_

#include <string>

class ContextManager {
public:
    static ContextManager &get();
    static void set(ContextManager *context);

    [[noreturn]]
    virtual void die(const std::string &error) = 0;
    virtual void terminate() = 0;
private:
    static ContextManager *context_;
};

#endif //LIBSBOX_CONTEXT_MANAGER_H_
