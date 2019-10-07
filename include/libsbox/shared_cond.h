/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_SHARED_COND_H_
#define LIBSBOX_SHARED_COND_H_

#include <libsbox/shared_mutex.h>

#include <unistd.h>

class SharedCond {
public:
    SharedCond();
    ~SharedCond();
    void wait(SharedMutex *mutex);
    void notify();
    void notify_all();
private:
    SharedMemory<pthread_cond_t> cond_;
    pid_t owner_pid_;
};

#endif //LIBSBOX_SHARED_COND_H_
