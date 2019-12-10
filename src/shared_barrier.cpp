/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include "shared_barrier.h"
#include "context_manager.h"
#include "utils.h"

#include <unistd.h>

SharedBarrier::SharedBarrier(uint32_t count) {
    barrier_ = std::make_unique<SharedMemoryObject<pthread_barrier_t>>();
    init(count);
    owner_pid_ = getpid();
}

SharedBarrier::~SharedBarrier() {
    if (getpid() == owner_pid_) {
        destroy();
    }
}

void SharedBarrier::wait() {
    int err = pthread_barrier_wait(barrier_->get());
    if (err != 0 && err != PTHREAD_BARRIER_SERIAL_THREAD) {
        die(format("Failed to wait() on barrier: %m"));
    }
}

void SharedBarrier::reset(uint32_t count) {
    destroy();
    init(count);
}

void SharedBarrier::init(uint32_t count) {
    pthread_barrierattr_t barrierattr;
    if (pthread_barrierattr_init(&barrierattr) != 0) {
        die(format("Failed to initialize barrierattr: %m"));
    }
    if (pthread_barrierattr_setpshared(&barrierattr, PTHREAD_PROCESS_SHARED) != 0) {
        die(format("Failed to set PTHREAD_PROCESS_SHARED to barrierattr: %m"));
    }
    if (pthread_barrier_init(barrier_->get(), &barrierattr, count)) {
        die(format("Failed to initialize barrier: %m"));
    }
    if (pthread_barrierattr_destroy(&barrierattr) != 0) {
        die(format("Failed to destroy barrierattr: %m"));
    }
}

void SharedBarrier::destroy() {
    if (pthread_barrier_destroy(barrier_->get()) != 0) {
        die(format("Failed to destroy barrier: %m"));
    }
}
