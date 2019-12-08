import subprocess
from tests import tests
from threading import Lock
import time
from sys import argv
import os


class Color:
    FAIL = '\033[31;1m'
    OK = '\033[92m'
    WARN = '\033[33;1m'
    END = '\033[0m'

    @staticmethod
    def ok(s):
        return Color.col(Color.OK, s)

    @staticmethod
    def warn(s):
        return Color.col(Color.WARN, s)

    @staticmethod
    def fail(s):
        return Color.col(Color.FAIL, s)

    @staticmethod
    def col(c, s):
        return c + s + Color.END


write_mutex = Lock()
failed = False


def safe_print(s):
    with write_mutex:
        print(s)


def test_passed(s):
    safe_print("[ " + Color.ok("PASSED") + " ] " + s)


def test_failed(s, desc):
    global failed
    failed = True
    safe_print("[ " + Color.fail("FAILED") + " ] " + s + "\n" + desc)


def format_argv(argv):
    return ' '.join([argv[i] if i == 0 else '"' + argv[i].replace('"', '\\"') + '"' for i in range(len(argv))])


def run_test(test):
    completed_process = subprocess.run(test.argv, stderr=subprocess.PIPE)
    if completed_process.returncode == 0:
        test_passed(format_argv(test.argv))
    else:
        test_failed(format_argv(test.argv), completed_process.stderr.decode())


def main():
    use_bundled = ("--use-bundled" in argv)

    if use_bundled:
        libsboxd_process = subprocess.Popen(["libsboxd"])
        time.sleep(0.5)
        assert libsboxd_process.poll() is None
    else:
        assert os.path.exists("/etc/libsboxd/socket")

    for test in tests:
        run_test(test)

    if use_bundled:
        os.remove("/run/libsboxd.pid")


if __name__ == "__main__":
    main()
