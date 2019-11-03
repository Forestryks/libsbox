/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_SHARED_MEMORY_H_
#define LIBSBOX_SHARED_MEMORY_H_

#include <stddef.h>

// Allocate memory which will be shared between descendant processes
void *allocate_shared_memory(size_t size);

// Deallocate shared memory
int free_shared_memory(void *ptr, size_t size);

#endif //LIBSBOX_SHARED_MEMORY_H_
