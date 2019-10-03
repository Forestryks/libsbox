/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_SHARED_COND_H_
#define LIBSBOX_SHARED_COND_H_

#include <libsbox/shared_memory.h>
#include <libsbox/shared_mutex.h>

#include <unistd.h>

class SharedCond {
public:
    SharedCond();
    ~SharedCond();
    void wait(SharedMutex *mutex);
    void notify();
    void notify_all();
    void destoy();
private:
    SharedMemory *shared_memory_;
    pthread_cond_t *cond_;
};

#endif //LIBSBOX_SHARED_COND_H_
