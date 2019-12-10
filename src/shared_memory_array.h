/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_SHARED_MEMORY_ARRAY_H
#define LIBSBOX_SHARED_MEMORY_ARRAY_H

#include "shared_memory.h"
#include "utils.h"
#include "context_manager.h"

#include <stddef.h>
#include <sys/mman.h>

// Class that holds array of objects created in shared memory
template<typename T>
class SharedMemoryArray {
public:
    explicit SharedMemoryArray(size_t size) : size_(size) {
        ptr_ = static_cast<T *>(allocate_shared_memory(sizeof(T) * size));
        ptr_ = new(ptr_) T[size]();
        if (ptr_ == MAP_FAILED) {
            die(format("Cannot allocate %zu bytes of shared memory: %m", sizeof(T)));
        }
    }

    ~SharedMemoryArray() {
        if (free_shared_memory(ptr_, sizeof(T)) != 0) {
            die(format("Cannot free shared memory at %p: %m", ptr_));
        }
    }

    T *get() {
        return ptr_;
    }

    size_t size() {
        return size_;
    }

    T &operator[](size_t index) {
        return ptr_[index];
    }

private:
    T *ptr_;
    size_t size_;
};

#endif //LIBSBOX_SHARED_MEMORY_ARRAY_H
