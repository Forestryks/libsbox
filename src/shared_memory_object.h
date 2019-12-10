/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_SHARED_MEMORY_OBJECT_H
#define LIBSBOX_SHARED_MEMORY_OBJECT_H

#include "shared_memory.h"
#include "utils.h"
#include "context_manager.h"

#include <sys/mman.h>

// Class that holds object created in shared memory
template<typename T>
class SharedMemoryObject {
public:
    template<typename ...Args>
    explicit SharedMemoryObject(Args &&... args) {
        ptr_ = static_cast<T *>(allocate_shared_memory(sizeof(T)));
        ptr_ = new(ptr_) T(args...);
        if (ptr_ == MAP_FAILED) {
            die(format("Cannot allocate %zu bytes of shared memory: %m", sizeof(T)));
        }
    }

    ~SharedMemoryObject() {
        if (free_shared_memory(ptr_, sizeof(T)) != 0) {
            die(format("Cannot free shared memory at %p: %m", ptr_));
        }
    }

    T *get() {
        return ptr_;
    }

    T *operator->() {
        return ptr_;
    }

private:
    T *ptr_;
};

#endif //LIBSBOX_SHARED_MEMORY_OBJECT_H
