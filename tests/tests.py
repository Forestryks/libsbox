import signal


class Test:
    def __init__(self, argv):
        self.argv = argv


tests = []

for exit_code in range(256):
    tests.append(Test(["./test_validate_exit_code", "invoker", str(exit_code)]))

abort_signals = [signal.SIGABRT, signal.SIGALRM, signal.SIGFPE, signal.SIGILL, signal.SIGINT, signal.SIGKILL,
                 signal.SIGPIPE, signal.SIGPOLL, signal.SIGPROF, signal.SIGQUIT, signal.SIGSEGV, signal.SIGSYS,
                 signal.SIGTERM, signal.SIGTRAP, signal.SIGUSR1, signal.SIGUSR2, signal.SIGVTALRM, signal.SIGXCPU,
                 signal.SIGXFSZ]
ignored_signals = [signal.SIGCHLD, signal.SIGURG]

for signal in abort_signals:
    tests.append(Test(["./test_validate_term_signal", "invoker", str(int(signal)), "terminated"]))

for signal in ignored_signals:
    tests.append(Test(["./test_validate_term_signal", "invoker", str(int(signal)), "exited"]))
