/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_SHARED_MUTEX_H_
#define LIBSBOX_SHARED_MUTEX_H_

#include <libsbox/shared_memory.h>
class SharedMutex;
#include <libsbox/shared_cond.h>

#include <unistd.h>

class SharedMutex {
public:
    SharedMutex();
    ~SharedMutex();
    void lock();
    void unlock();
    void destroy();
private:
    SharedMemory *shared_memory_;
    pthread_mutex_t *mutex_;

    friend class SharedCond;
};

#endif //LIBSBOX_SHARED_MUTEX_H_
