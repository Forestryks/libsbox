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
    SharedIdGetter(uid_t start, uid_t count);
    ~SharedIdGetter() = default;

    // Get new unique ID, will fail if no free IDs left
    uid_t get();

    // Put given ID back
    void put(uid_t id);
private:
    std::unique_ptr<SharedMemoryArray<uid_t>> ids_stack_;
    std::unique_ptr<SharedMemoryObject<size_t>> stack_head_;
    SharedMutex mutex_;
};

#endif //LIBSBOX_SHARED_ID_GETTER_H
