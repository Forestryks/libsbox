#include <iostream>
#include <cstring>

#include "libsbox.h"

using namespace std;

static void optimizer_protect(void *p) {
    asm volatile("" : : "g"(p) : "memory");
}

static void optimizer_protect_all() {
    asm volatile("" : : : "memory");
}

void massert_internal(bool cond, int line, const char *msg) {
    if (!cond) {
        cout << "Assertation (" << msg << ") [line:" << line << "] failed: " << strerror(errno) << endl;
        exit(1);
    }
}

#define massert(cond) massert_internal(cond, __LINE__, #cond)

void run() {
    libsbox::Task task;
    task.set_argv({"/home/forestryks/Projects/libsbox/testing/test", "hello"});
    task.set_time_limit_ms(1000);
    task.set_wall_time_limit_ms(1000);
    task.set_memory_limit_kb(262144);
    task.set_fsize_limit_kb(262144);
    task.set_max_files(16);
    task.set_max_threads(1);
    task.get_stdin().use_file("input");
    task.get_stdout().use_file("output");
    task.get_stderr().disable();
    task.set_need_ipc(false);
    task.set_use_standard_binds(true);
    task.add_bind(libsbox::Bind(".", "/home/forestryks/Projects/libsbox/testing/work").allow_write());

    try {
        if (libsbox::run({&task}) != 0) {
            std::cout << "Error: " << strerror(errno) << std::endl;
        }
    } catch (nlohmann::json::exception &e) {
        std::cout << e.what() << std::endl;
    }

    std::cout << "time_usage_ms: " << task.get_time_usage_ms() << std::endl;
    std::cout << "time_usage_user_ms: " << task.get_time_usage_user_ms() << std::endl;
    std::cout << "time_usage_sys_ms: " << task.get_time_usage_sys_ms() << std::endl;
    std::cout << "wall_time_usage_ms: " << task.get_wall_time_usage_ms() << std::endl;
    std::cout << "memory_usage_kb: " << task.get_memory_usage_kb() << std::endl;
    std::cout << "exited?: " << task.exited() << std::endl;
    std::cout << "exit_code: " << task.get_exit_code() << std::endl;
    std::cout << "signaled?: " << task.is_signaled() << std::endl;
    std::cout << "term_signal: " << task.get_term_signal() << std::endl;
    std::cout << "oom_killed?: " << task.is_oom_killed() << std::endl;
    std::cout << "memory_limit_hit?: " << task.is_memory_limit_hit() << std::endl;
}

int main(int argc, char *argv[]) {
    int cnt = 1;
    if (argc == 2) {
        cnt = atoi(argv[1]);
    }

    for (int i = 0; i < cnt; ++i) {
        std::cout << i + 1 << std::endl;
        run();
        std::cout << std::endl;
    }
}
