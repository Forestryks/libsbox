/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_SHARED_COUNTER_H_
#define LIBSBOX_SHARED_COUNTER_H_

#include <libsbox/shared_mutex.h>

class SharedCounter {
public:
    explicit SharedCounter(int val);
    ~SharedCounter() = default;
    void inc();
    int get();
    int get_and_inc();
    int inc_and_get();
    void set(int val);
private:
    SharedMutex mutex_;
    SharedMemory<int> val_;
};

#endif //LIBSBOX_SHARED_COUNTER_H_