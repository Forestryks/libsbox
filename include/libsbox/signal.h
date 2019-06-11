/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_SIGNAL_H
#define LIBSBOX_SIGNAL_H

namespace libsbox {
    void prepare_signals();
    void reset_signals();

    void start_timer(long interval);
    void stop_timer();
    extern int interrupt_signal;
} // namespace libsbox

#endif //LIBSBOX_SIGNAL_H
