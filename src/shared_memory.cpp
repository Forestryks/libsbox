/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include "shared_memory.h"

#include <sys/mman.h>

void *allocate_shared_memory(size_t size) {
    return mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
}

int free_shared_memory(void *ptr, size_t size) {
    return munmap(ptr, size);
}
