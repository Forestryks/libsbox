/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include <libsbox/shared_cond.h>

SharedCond::SharedCond() {
    shared_memory_ = new SharedMemory(sizeof(pthread_cond_t));
    cond_ = (pthread_cond_t *) shared_memory_->get();

    pthread_condattr_t condattr;
    if (pthread_condattr_init(&condattr) != 0) {
        Context::get().die(format("Failed to initialize condattr: %m"));
    }
    if (pthread_condattr_setpshared(&condattr, PTHREAD_PROCESS_SHARED) != 0) {
        Context::get().die(format("Failed to set PTHREAD_PROCESS_SHARED to condattr: %m"));
    }
    if (pthread_cond_init(cond_, &condattr)) {
        Context::get().die(format("Failed to initialize cond: %m"));
    }
    if (pthread_condattr_destroy(&condattr) != 0) {
        Context::get().die(format("Failed to destroy condattr: %m"));
    }
}

SharedCond::~SharedCond() {
    delete shared_memory_;
}

void SharedCond::wait(SharedMutex *mutex) {
    if (pthread_cond_wait(cond_, mutex->mutex_) != 0) {
        Context::get().die(format("Failed to wait() cond: %m"));
    }
}

void SharedCond::notify() {
    if (pthread_cond_signal(cond_) != 0) {
        Context::get().die(format("Failed to notify() cond: %m"));
    }
}

void SharedCond::notify_all() {
    if (pthread_cond_broadcast(cond_) != 0) {
        Context::get().die(format("Failed to notify_all() cond: %m"));
    }
}

void SharedCond::destoy() {
    if (pthread_cond_destroy(cond_) != 0) {
        Context::get().die(format("Failed to destroy cond: %m"));
    }
}
