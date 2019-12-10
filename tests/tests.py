import signal


class Test:
    def __init__(self, argv, optional=False):
        self.argv = argv
        self.optional = optional


tests = []

for exit_code in range(256):
    tests.append(Test(["./test_exit_code", "invoker", str(exit_code)]))

abort_signals = [signal.SIGABRT, signal.SIGALRM, signal.SIGFPE, signal.SIGILL, signal.SIGINT, signal.SIGKILL,
                 signal.SIGPIPE, signal.SIGPOLL, signal.SIGPROF, signal.SIGQUIT, signal.SIGSEGV, signal.SIGSYS,
                 signal.SIGTERM, signal.SIGTRAP, signal.SIGUSR1, signal.SIGUSR2, signal.SIGVTALRM, signal.SIGXCPU,
                 signal.SIGXFSZ]
ignored_signals = [signal.SIGCHLD, signal.SIGURG]


for signal in abort_signals:
    tests.append(Test(["./test_term_signal", "invoker", str(int(signal)), "terminated"]))

for signal in ignored_signals:
    tests.append(Test(["./test_term_signal", "invoker", str(int(signal)), "exited"]))

for time_limit in (10, 25, 50, 100, 200, 500):
    tests.append(Test(["./test_time_limit", "invoker", str(time_limit)]))

for time_limit in (10, 25, 50, 100, 200, 500):
    tests.append(Test(["./test_wall_time_limit", "invoker", str(time_limit)]))

for memory_limit in (16 * 1024, 256 * 1024, 512 * 1024, 1024 * 1024):
    tests.append(Test(["./test_memory_limit", "invoker", str(memory_limit)]))

for target_time_usage in (10, 20, 30, 40, 50, 100, 250, 500, 1000):
    for allowed_delta in (100, 50, 25):
        tests.append(Test(["./test_time_usage", "invoker", str(target_time_usage), str(allowed_delta)]))
    for allowed_delta in (15, 10, 5, 3, 2, 1, 0):
        tests.append(Test(["./test_time_usage", "invoker", str(target_time_usage), str(allowed_delta)], optional=True))

for target_time_usage in (10, 20, 30, 40, 50, 100, 250, 500, 1000):
    for allowed_delta in (100, 50, 25):
        tests.append(Test(["./test_wall_time_usage", "invoker", str(target_time_usage), str(allowed_delta)]))
    for allowed_delta in (15, 10, 5, 3, 2, 1, 0):
        tests.append(Test(["./test_wall_time_usage", "invoker", str(target_time_usage), str(allowed_delta)], optional=True))

for memory_limit in (16 * 1024, 256 * 1024, 512 * 1024, 1024 * 1024):
    tests.append(Test(["./test_memory_usage", "invoker", str(memory_limit), str(4096)]))
    tests.append(Test(["./test_memory_usage", "invoker", str(memory_limit), str(2048)], optional=True))
    tests.append(Test(["./test_memory_usage", "invoker", str(memory_limit), str(1024)], optional=True))
