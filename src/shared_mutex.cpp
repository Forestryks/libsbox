/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include <libsbox/shared_mutex.h>

SharedMutex::SharedMutex() {
    shared_memory_ = new SharedMemory(sizeof(pthread_mutex_t));
    mutex_ = (pthread_mutex_t *) shared_memory_->get();

    pthread_mutexattr_t mutexattr;
    if (pthread_mutexattr_init(&mutexattr) != 0) {
        Context::get().die(format("Failed to initialize mutexattr: %m"));
    }
    if (pthread_mutexattr_setpshared(&mutexattr, PTHREAD_PROCESS_SHARED) != 0) {
        Context::get().die(format("Failed to set PTHREAD_PROCESS_SHARED to mutexattr: %m"));
    }
    if (pthread_mutex_init(mutex_, &mutexattr)) {
        Context::get().die(format("Failed to initialize mutex: %m"));
    }
    if (pthread_mutexattr_destroy(&mutexattr) != 0) {
        Context::get().die(format("Failed to destroy mutexattr: %m"));
    }
}

SharedMutex::~SharedMutex() {
    delete shared_memory_;
}

void SharedMutex::lock() {
    if (pthread_mutex_lock(mutex_) != 0) {
        Context::get().die(format("Failed to lock mutex: %m"));
    }
}

void SharedMutex::unlock() {
    if (pthread_mutex_unlock(mutex_) != 0) {
        Context::get().die(format("Failed to unlock mutex: %m"));
    }
}

void SharedMutex::destroy() {
    if (pthread_mutex_destroy(mutex_) != 0) {
        Context::get().die(format("Failed to destroy mutex: %m"));
    }
}

