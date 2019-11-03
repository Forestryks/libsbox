/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_SHARED_COND_H_
#define LIBSBOX_SHARED_COND_H_

#include "shared_mutex.h"
#include "shared_memory_object.h"

#include <pthread.h>
#include <stddef.h>
#include <memory>

// Multiprocess condition variable
class SharedCond {
public:
    SharedCond();
    ~SharedCond();
    void wait(SharedMutex *mutex);
    void notify();
    void notify_all();
private:
    std::unique_ptr<SharedMemoryObject<pthread_cond_t>> cond_;
    pid_t owner_pid_;
};

#endif //LIBSBOX_SHARED_COND_H_
