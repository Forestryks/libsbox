/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_SHARED_MEMORY_H_
#define LIBSBOX_SHARED_MEMORY_H_

#include <libsbox/context.h>
#include <libsbox/utils.h>

#include <pthread.h>
#include <sys/mman.h>

void *allocate_shared_memory(size_t size);

int free_shared_memory(void *ptr, size_t size);

class SharedMemory {
public:
    explicit SharedMemory(size_t size);
    ~SharedMemory();
    void *get();
private:
    size_t size_;
    void *ptr_;
};

#endif //LIBSBOX_SHARED_MEMORY_H_
