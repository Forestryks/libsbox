/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include <libsbox/signals.h>
#include <libsbox/context.h>
#include <libsbox/utils.h>

#include <sys/signal.h>
#include <sys/time.h>
#include <cstring>

enum sigactions {
    SIGACTION_IGNORE,
    SIGACTION_INTERRUPT,
    SIGACTION_TERMINATE
};

struct signal_action {
    int signum;
    sigactions sigaction;
};

const struct signal_action signal_actions[] = {
    {SIGUSR1, SIGACTION_IGNORE},
    {SIGUSR2, SIGACTION_IGNORE},
    {SIGPIPE, SIGACTION_IGNORE},
    {SIGHUP,  SIGACTION_TERMINATE},
    {SIGQUIT, SIGACTION_TERMINATE},
    {SIGILL,  SIGACTION_TERMINATE},
    {SIGABRT, SIGACTION_TERMINATE},
    {SIGIOT,  SIGACTION_TERMINATE},
    {SIGBUS,  SIGACTION_TERMINATE},
    {SIGFPE,  SIGACTION_TERMINATE},
    {SIGINT,  SIGACTION_TERMINATE},
    {SIGTERM, SIGACTION_INTERRUPT},
    {SIGALRM, SIGACTION_INTERRUPT}
};

int interrupt_signal;

// SIGALRM and SIGTERM must be handled synchronously
void sigaction_interrupt_handler(int signum) {
    interrupt_signal = signum;
}

// We want to try dieing gracefully when critical signal is caught
void sigaction_terminate_handler(int signum) {
    Context::get().die(format("Received fatal signal %d (%s)", signum, strsignal(signum)));
}

void prepare_signals() {
    struct sigaction sigaction_interrupt = {},
        sigaction_terminate = {},
        sigaction_ignore = {};
    memset(&sigaction_interrupt, 0, sizeof(sigaction_interrupt));
    memset(&sigaction_terminate, 0, sizeof(sigaction_terminate));
    memset(&sigaction_ignore, 0, sizeof(sigaction_ignore));
    sigaction_interrupt.sa_handler = sigaction_interrupt_handler;
    sigaction_terminate.sa_handler = sigaction_terminate_handler;
    sigaction_ignore.sa_handler = SIG_IGN;

    for (auto &signal_action : signal_actions) {
        switch (signal_action.sigaction) {
            case SIGACTION_IGNORE:
                if (sigaction(signal_action.signum, &sigaction_ignore, nullptr)) {
                    Context::get().die(format("Cannot set signal action for %s: %m", strsignal(signal_action.signum)));
                }
                break;
            case SIGACTION_INTERRUPT:
                if (sigaction(signal_action.signum, &sigaction_interrupt, nullptr)) {
                    Context::get().die(format("Cannot set signal action for %s: %m", strsignal(signal_action.signum)));
                }
                break;
            case SIGACTION_TERMINATE:
                if (sigaction(signal_action.signum, &sigaction_terminate, nullptr)) {
                    Context::get().die(format("Cannot set signal action for %s: %m", strsignal(signal_action.signum)));
                }
                break;
            default:
                Context::get().die("Invalid signal action");
        }
    }
}

void reset_signals() {
    struct sigaction sigaction_default = {};
    memset(&sigaction_default, 0, sizeof(sigaction_default));
    sigaction_default.sa_handler = SIG_DFL;

    for (auto &signal_action : signal_actions) {
        if (sigaction(signal_action.signum, &sigaction_default, nullptr)) {
            Context::get().die(format("Cannot restore default signal action for %s (%m)", strsignal(signal_action.signum)));
        }
    }
}

void start_timer(long interval) {
    struct itimerval timer = {};
    memset(&timer, 0, sizeof(timer));
    timer.it_interval.tv_usec = interval * 1000;
    timer.it_value.tv_usec = interval * 1000;
    setitimer(ITIMER_REAL, &timer, nullptr);
}

void stop_timer() {
    struct itimerval timer = {};
    memset(&timer, 0, sizeof(timer));
    setitimer(ITIMER_REAL, &timer, nullptr);
}
