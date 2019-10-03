/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include <libsbox/shared_counter.h>

SharedCounter::SharedCounter(int val) {
    shared_memory_ = new SharedMemory(sizeof(int));
    val_ = (int *) shared_memory_->get();
    (*val_) = val;
}

SharedCounter::~SharedCounter() {
    delete shared_memory_;
}

void SharedCounter::inc() {
    mutex_.lock();
    (*val_)++;
    mutex_.unlock();
}

int SharedCounter::get() {
    mutex_.lock();
    int ret = (*val_);
    mutex_.unlock();
    return ret;
}

int SharedCounter::get_and_inc() {
    mutex_.lock();
    int ret = (*val_)++;
    mutex_.unlock();
    return ret;
}

void SharedCounter::destroy() {
    mutex_.destroy();
}

void SharedCounter::set(int val) {
    mutex_.lock();
    (*val_) = val;
    mutex_.unlock();
}
