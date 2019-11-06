/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include "shared_id_getter.h"
#include "context_manager.h"
#include "utils.h"

#include <mutex>

SharedIdGetter::SharedIdGetter(int start, int count) {
    ids_stack_ = std::make_unique<SharedMemoryArray<int>>(count);
    stack_head_ = std::make_unique<SharedMemoryObject<int>>();
    for (int i = 0; i < count; ++i) {
        (*ids_stack_)[i] = start + count - 1 - i; // Lower the ID is, closer to stack head it is
    }
    (*stack_head_->get()) = count - 1;
}

int SharedIdGetter::get() {
    std::unique_lock lock(mutex_);
    if ((*stack_head_->get()) == -1) {
        die(format("Failed to get ID: stack is empty"));
    }
    int ret = (*ids_stack_)[*stack_head_->get()];
    (*stack_head_->get())--;
    return ret;
}

void SharedIdGetter::put(int id) {
    std::unique_lock lock(mutex_);
    if ((*stack_head_->get()) + 1 == static_cast<long>(ids_stack_->size())) {
        die(format("Failed to put ID (%d): stack is full", id));
    }
    (*stack_head_->get())++;
    (*ids_stack_)[*stack_head_->get()] = id;
}
