/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include "shared_id_getter.h"
#include "context_manager.h"
#include "utils.h"

#include <mutex>

SharedIdGetter::SharedIdGetter(uid_t start, uid_t count) {
    ids_stack_ = std::make_unique<SharedMemoryArray<uid_t>>(count);
    stack_head_ = std::make_unique<SharedMemoryObject<size_t>>();
    for (size_t i = 0; i < count; ++i) {
        (*ids_stack_)[i] = start + count - 1 - i; // Lower the ID is, closer to stack head it is
    }
    (*stack_head_->get()) = count;
}

uid_t SharedIdGetter::get() {
    std::unique_lock lock(mutex_);
    if ((*stack_head_->get()) == 0) {
        die(format("Failed to get ID: stack is empty"));
    }
    (*stack_head_->get())--;
    return (*ids_stack_)[*stack_head_->get()];
}

void SharedIdGetter::put(uid_t id) {
    std::unique_lock lock(mutex_);
    if (*stack_head_->get() == ids_stack_->size()) {
        die(format("Failed to put ID (%d): stack is full", id));
    }
    (*ids_stack_)[*stack_head_->get()] = id;
    (*stack_head_->get())++;
}
