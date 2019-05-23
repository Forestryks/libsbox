/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_SIGNAL_H
#define LIBSBOX_SIGNAL_H

namespace libsbox {
    void disable_signals();
    void reset_signals();

    extern int interrupt_signal;
} // namespace libsbox

#endif //LIBSBOX_SIGNAL_H
