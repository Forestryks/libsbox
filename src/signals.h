/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_SIGNALS_H
#define LIBSBOX_SIGNALS_H

// Prepare signal handlers
void prepare_signals();

// Reset signal handlers to system defaults (SIG_DFL)
void reset_signals();

// Set signal handler of signum to SA_RESTART if restart is true, clear bit otherwise
void set_standard_handler_restart(int signum, bool restart);

// Start timer which will send SIGALRM every interval_ms milliseconds
void start_timer(long interval_ms);
void stop_timer();

// Can be checked after signal handler returns, if true, SIGALRM was received
extern bool timer_interrupt;

#endif //LIBSBOX_SIGNALS_H
