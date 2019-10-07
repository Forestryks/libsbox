/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include <libsbox/context_manager.h>

ContextManager *ContextManager::context_ = nullptr;

ContextManager &ContextManager::get() {
    return *context_;
}

void ContextManager::set(ContextManager *context) {
    context_ = context;
}
