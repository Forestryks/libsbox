/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_SHARED_BARRIER_H_
#define LIBSBOX_SHARED_BARRIER_H_

#include <libsbox/shared_mutex.h>
#include <libsbox/shared_memory.h>
#include <libsbox/shared_cond.h>

class SharedBarrier {
public:
    explicit SharedBarrier();
    ~SharedBarrier();
    void sub();
    void wait(int val);
    void destroy();
private:
    SharedMutex mutex_;
    SharedCond cond_;
    SharedMemory *shared_memory_;
    int *val_;
};

#endif //LIBSBOX_SHARED_BARRIER_H_
