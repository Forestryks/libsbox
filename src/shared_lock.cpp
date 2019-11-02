/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include "shared_lock.h"

SharedLock::SharedLock(SharedMutex &mutex) {
    mutex_ = &mutex;
    mutex_->lock();
}

SharedLock::~SharedLock() {
    mutex_->unlock();
}
