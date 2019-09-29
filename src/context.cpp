/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include <libsbox/context.h>

Context *Context::context_ = nullptr;

Context &Context::get() {
    return *context_;
}

void Context::set_context(Context *context) {
    context_ = context;
}
