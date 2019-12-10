/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include "shared_mutex.h"
#include "context_manager.h"

#include <unistd.h>
#include <memory>

SharedMutex::SharedMutex() {
    mutex_ = std::make_unique<SharedMemoryObject<pthread_mutex_t>>();
    pthread_mutexattr_t mutexattr;
    if (pthread_mutexattr_init(&mutexattr) != 0) {
        die(format("Failed to initialize mutexattr: %m"));
    }
    if (pthread_mutexattr_setpshared(&mutexattr, PTHREAD_PROCESS_SHARED) != 0) {
        die(format("Failed to set PTHREAD_PROCESS_SHARED to mutexattr: %m"));
    }
    if (pthread_mutex_init(mutex_->get(), &mutexattr)) {
        die(format("Failed to initialize mutex: %m"));
    }
    if (pthread_mutexattr_destroy(&mutexattr) != 0) {
        die(format("Failed to destroy mutexattr: %m"));
    }
    owner_pid_ = getpid();
}

SharedMutex::~SharedMutex() {
    if (getpid() == owner_pid_) {
        if (pthread_mutex_destroy(mutex_->get()) != 0) {
            die(format("Failed to destroy mutex: %m"));
        }
    }
}

void SharedMutex::lock() {
    if (pthread_mutex_lock(mutex_->get()) != 0) {
        die(format("Failed to lock mutex: %m"));
    }
}

void SharedMutex::unlock() {
    if (pthread_mutex_unlock(mutex_->get()) != 0) {
        die(format("Failed to unlock mutex: %m"));
    }
}

pthread_mutex_t *SharedMutex::get_mutex() {
    return mutex_->get();
}
