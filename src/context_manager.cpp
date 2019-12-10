/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include "context_manager.h"

ContextManager *ContextManager::context_ = nullptr;

ContextManager &ContextManager::get() {
    return *context_;
}

void ContextManager::set(ContextManager *context, const std::string &name) {
    context_ = context;
    context_->name_ = name;
}

const std::string &ContextManager::get_name() const {
    return name_;
}
