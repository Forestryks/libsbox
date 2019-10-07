/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_SHARED_WAITER_H_
#define LIBSBOX_SHARED_WAITER_H_

#include <libsbox/shared_memory.h>
#include <libsbox/shared_mutex.h>
#include <libsbox/shared_cond.h>

class SharedWaiter {
public:
    SharedWaiter();
    ~SharedWaiter() = default;
    void sub();
    void wait(int val);
private:
    SharedMutex mutex_;
    SharedCond cond_;
    SharedMemory<int> val_;
};

#endif //LIBSBOX_SHARED_WAITER_H_
