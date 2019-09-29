/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_SIGNALS_H_
#define LIBSBOX_SIGNALS_H_

void prepare_signals();
void reset_signals();

void start_timer(long interval);
void stop_timer();
extern int interrupt_signal;

#endif //LIBSBOX_SIGNALS_H_
