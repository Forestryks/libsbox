/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include <libsbox/shared_memory.h>

SharedMemory::SharedMemory(size_t size) {
    ptr_ = allocate_shared_memory(size);
    if (ptr_ == MAP_FAILED) {
        Context::get().die(format("Cannot allocate %zu bytes of shared memory: %m", size));
    }
    size_ = size;
}

SharedMemory::~SharedMemory() {
    if (free_shared_memory(ptr_, size_) != 0) {
        Context::get().die(format("Cannot free shared memory at %p: %m", ptr_));
    }
}

void *SharedMemory::get() {
    return ptr_;
}

void *allocate_shared_memory(size_t size) {
    return mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
}

int free_shared_memory(void *ptr, size_t size) {
    return munmap(ptr, size);
}
