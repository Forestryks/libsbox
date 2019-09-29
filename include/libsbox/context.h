/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_CONTEXT_H_
#define LIBSBOX_CONTEXT_H_

#include <string>

// Purpose of this class is to provide single interface to die() from error handlers in different processes
class Context {
public:
    static Context &get();
    static void set_context(Context *context);

    [[noreturn]]
    virtual void die(const std::string &error) = 0;
private:
    static Context *context_;
};

#endif //LIBSBOX_CONTEXT_H_
