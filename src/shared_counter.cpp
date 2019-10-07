/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include <libsbox/shared_counter.h>

#include <mutex>

SharedCounter::SharedCounter(int val) {
    val_.set(val);
}

void SharedCounter::inc() {
    std::unique_lock<SharedMutex> lock(mutex_);
    val_.get()++;
}

int SharedCounter::get() {
    std::unique_lock<SharedMutex> lock(mutex_);
    return val_.get();
}

int SharedCounter::get_and_inc() {
    std::unique_lock<SharedMutex> lock(mutex_);
    return val_.get()++;
}

int SharedCounter::inc_and_get() {
    std::unique_lock<SharedMutex> lock(mutex_);
    return ++val_.get();
}

void SharedCounter::set(int val) {
    std::unique_lock<SharedMutex> lock(mutex_);
    val_.get() = val;
}
