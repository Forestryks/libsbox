/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_SHARED_ID_GETTER_H
#define LIBSBOX_SHARED_ID_GETTER_H

#include "shared_memory_array.h"
#include "shared_memory_object.h"
#include "shared_mutex.h"

#include <memory>

// Multiprocess unique ID getter
class SharedIdGetter {
public:
    // Initialize getter with IDs in segment [start, start + count - 1]
    SharedIdGetter(int start, int count);
    ~SharedIdGetter() = default;

    // Get new unique ID, will fail if no free IDs left
    int get();

    // Put given ID back
    void put(int id);
private:
    std::unique_ptr<SharedMemoryArray<int>> ids_stack_;
    std::unique_ptr<SharedMemoryObject<int>> stack_head_;
    SharedMutex mutex_;
};

#endif //LIBSBOX_SHARED_ID_GETTER_H
