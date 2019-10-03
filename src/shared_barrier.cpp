/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include <libsbox/shared_barrier.h>

#include <iostream>

SharedBarrier::SharedBarrier() {
    shared_memory_ = new SharedMemory(sizeof(int));
    val_ = (int *) shared_memory_->get();
    (*val_) = 0;
}

SharedBarrier::~SharedBarrier() {
    delete shared_memory_;
}

void SharedBarrier::sub() {
    mutex_.lock();
    (*val_)--;
    if ((*val_) <= 0) cond_.notify();
    mutex_.unlock();
}

void SharedBarrier::wait(int val) {
    mutex_.lock();
    (*val_) += val;
    if ((*val_) <= 0) {
        mutex_.unlock();
        return;
    }
    cond_.wait(&mutex_);
    mutex_.unlock();
}

void SharedBarrier::destroy() {
    mutex_.destroy();
    cond_.destoy();
}
