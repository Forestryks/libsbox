/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include "shared_cond.h"

SharedCond::SharedCond() {
    cond_ = std::make_unique<SharedMemoryObject<pthread_cond_t>>();
    pthread_condattr_t condattr;
    if (pthread_condattr_init(&condattr) != 0) {
        ContextManager::get().die(format("Failed to initialize condattr: %m"));
    }
    if (pthread_condattr_setpshared(&condattr, PTHREAD_PROCESS_SHARED) != 0) {
        ContextManager::get().die(format("Failed to set PTHREAD_PROCESS_SHARED to condattr: %m"));
    }
    if (pthread_cond_init(cond_->get(), &condattr)) {
        ContextManager::get().die(format("Failed to initialize cond: %m"));
    }
    if (pthread_condattr_destroy(&condattr) != 0) {
        ContextManager::get().die(format("Failed to destroy condattr: %m"));
    }
    owner_pid_ = getpid();
}

SharedCond::~SharedCond() {
    if (getpid() == owner_pid_) {
        if (pthread_cond_destroy(cond_->get()) != 0) {
            ContextManager::get().die(format("Failed to destroy cond: %m"));
        }
    }
}

void SharedCond::wait(SharedMutex *mutex) {
    if (pthread_cond_wait(cond_->get(), mutex->get_mutex()) != 0) {
        ContextManager::get().die(format("Failed to wait() cond: %m"));
    }
}

void SharedCond::notify() {
    if (pthread_cond_signal(cond_->get()) != 0) {
        ContextManager::get().die(format("Failed to notify() cond: %m"));
    }
}

void SharedCond::notify_all() {
    if (pthread_cond_broadcast(cond_->get()) != 0) {
        ContextManager::get().die(format("Failed to notify_all() cond: %m"));
    }
}
