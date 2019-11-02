/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_SHARED_LOCK_H
#define LIBSBOX_SHARED_LOCK_H

#include "shared_mutex.h"

class SharedLock {
public:
    explicit SharedLock(SharedMutex &mutex);
    ~SharedLock();
private:
    SharedMutex *mutex_;
};

#endif //LIBSBOX_SHARED_LOCK_H
