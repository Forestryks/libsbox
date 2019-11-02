/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_SHARED_BARRIER_H
#define LIBSBOX_SHARED_BARRIER_H

#include "shared_memory_object.h"

#include <pthread.h>
#include <memory>

// TODO: ownership and reset?

// Multiprocess barrier
class SharedBarrier {
public:
    explicit SharedBarrier(int count);
    ~SharedBarrier();
    void wait();
    void reset(int count);
private:
    std::unique_ptr<SharedMemoryObject<pthread_barrier_t>> barrier_;
    pid_t owner_pid_;

    void init(int count);
    void destroy();
};

#endif //LIBSBOX_SHARED_BARRIER_H
