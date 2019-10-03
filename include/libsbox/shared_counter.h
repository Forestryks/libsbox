/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_SHARED_COUNTER_H_
#define LIBSBOX_SHARED_COUNTER_H_

#include <libsbox/shared_memory.h>
#include <libsbox/shared_mutex.h>

class SharedCounter {
public:
    explicit SharedCounter(int val);
    ~SharedCounter();
    void inc();
    int get();
    int get_and_inc();
    void set(int val);
    void destroy();
private:
    SharedMutex mutex_;
    SharedMemory *shared_memory_;
    int *val_;
};

#endif //LIBSBOX_SHARED_COUNTER_H_
