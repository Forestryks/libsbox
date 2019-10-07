/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include <libsbox/shared_waiter.h>

#include <mutex>

SharedWaiter::SharedWaiter() {
    val_.set(0);
}

void SharedWaiter::sub() {
    std::unique_lock<SharedMutex> lock(mutex_);
    val_.get()--;
    if (val_.get() <= 0) cond_.notify();
}

void SharedWaiter::wait(int val) {
    std::unique_lock<SharedMutex> lock(mutex_);
    val_.get() += val;
    if (val_.get() <= 0) return;
    cond_.wait(&mutex_);
}
