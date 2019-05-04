/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include <libsbox/signal.h>

#include <libsbox/die.h>

#include <signal.h>
#include <cstring>
#include <errno.h>

namespace libsbox {
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
            {SIGTERM, SIGACTION_INTERRUPT},
            {SIGHUP,  SIGACTION_INTERRUPT},
            {SIGINT,  SIGACTION_INTERRUPT},
            {SIGQUIT, SIGACTION_INTERRUPT},
            {SIGILL,  SIGACTION_TERMINATE},
            {SIGABRT, SIGACTION_TERMINATE},
            {SIGIOT,  SIGACTION_TERMINATE},
            {SIGBUS,  SIGACTION_TERMINATE},
            {SIGFPE,  SIGACTION_TERMINATE}
    };
}

int libsbox::interrupt_signal;

namespace libsbox {
    void sigaction_interrupt_handler(int signum) {
        interrupt_signal = signum;
    }

    void sigaction_terminate_handler(int signum) {
        panic("Received fatal signal %d (%s)", signum, strsignal(signum));
    }
}

void libsbox::disable_signals() {
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
                    die("Cannot set signal action for %s (%s)", strsignal(signal_action.signum), strerror(errno));
                }
                break;
            case SIGACTION_INTERRUPT:
                if (sigaction(signal_action.signum, &sigaction_interrupt, nullptr)) {
                    die("Cannot set signal action for %s (%s)", strsignal(signal_action.signum), strerror(errno));
                }
                break;
            case SIGACTION_TERMINATE:
                if (sigaction(signal_action.signum, &sigaction_terminate, nullptr)) {
                    die("Cannot set signal action for %s (%s)", strsignal(signal_action.signum), strerror(errno));
                }
                break;
            default:
                die("Invalid signal action");
        }
    }
}

void libsbox::restore_signals() {
    struct sigaction sigaction_default = {};
    memset(&sigaction_default, 0, sizeof(sigaction_default));
    sigaction_default.sa_handler = SIG_DFL;

    for (auto &signal_action : signal_actions) {
        if (sigaction(signal_action.signum, &sigaction_default, nullptr)) {
            die("Cannot restore default signal action for %s (%s)", strsignal(signal_action.signum), strerror(errno));
        }
    }
}
