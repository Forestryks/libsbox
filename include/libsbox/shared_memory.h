/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_SHARED_MEMORY_H_
#define LIBSBOX_SHARED_MEMORY_H_

#include <libsbox/utils.h>
#include <libsbox/context_manager.h>

#include <sys/mman.h>

void *allocate_shared_memory(size_t size);
int free_shared_memory(void *ptr, size_t size);

template<typename T>
class SharedMemory {
public:
    SharedMemory() {
        ptr_ = (T *) allocate_shared_memory(sizeof(T));
        if (ptr_ == MAP_FAILED) {
            ContextManager::get().die(format("Cannot allocate %zu bytes of shared memory: %m", sizeof(T)));
        }
    }

    ~SharedMemory() {
        if (free_shared_memory(ptr_, sizeof(T)) != 0) {
            ContextManager::get().die(format("Cannot free shared memory at %p: %m", ptr_));
        }
    }

    T &get() {
        return *ptr_;
    }

    void set(const T &val) {
        *ptr_ = val;
    }

    void set(const T &&val) {
        *ptr_ = val;
    }

    T *operator->() {
        return ptr_;
    }
private:
    T *ptr_;
};

#endif //LIBSBOX_SHARED_MEMORY_H_
