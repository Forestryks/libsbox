/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_SHARED_MUTEX_H_
#define LIBSBOX_SHARED_MUTEX_H_

#include <libsbox/shared_memory.h>
#include <unistd.h>

class SharedMutex {
public:
    SharedMutex();
    ~SharedMutex();
    void lock();
    void unlock();
    pthread_mutex_t *get_mutex();
private:
    SharedMemory<pthread_mutex_t> mutex_;
    pid_t owner_pid_;
};

#endif //LIBSBOX_SHARED_MUTEX_H_
