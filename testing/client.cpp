#include <iostream>
#include <cstring>
#include <linux/limits.h>

#include <libsbox.h>

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

std::string get_cwd() {
    char buf[PATH_MAX];
    massert(getcwd(buf, PATH_MAX) == buf);
    return buf;
}

void run() {
    std::string cwd = get_cwd();
    libsbox::Task task;
    task.set_argv({"./test", "hello"});
    task.set_time_limit_ms(1000);
    task.set_wall_time_limit_ms(1000);
    task.set_memory_limit_kb(50000);
    task.set_fsize_limit_kb(262144);
    task.set_max_files(16);
    task.set_max_threads(1);
    task.get_stdin().use_file("input");
    task.get_stdout().use_file("output");
    task.get_stderr().disable();
    task.set_need_ipc(false);
    task.set_use_standard_binds(true);
    task.get_binds().emplace_back(".", cwd + "/work").allow_write();

    auto error = libsbox::run_together({&task});
    if (error) {
        std::cout << error.get() << std::endl;
        _exit(1);
    }

    std::cout << "time_usage_ms: " << task.get_time_usage_ms() << std::endl;
    std::cout << "time_usage_user_ms: " << task.get_time_usage_user_ms() << std::endl;
    std::cout << "time_usage_sys_ms: " << task.get_time_usage_sys_ms() << std::endl;
    std::cout << "wall_time_usage_ms: " << task.get_wall_time_usage_ms() << std::endl;
    std::cout << "memory_usage_kb: " << task.get_memory_usage_kb() << std::endl;
    std::cout << "time_limit_exceeded? " << task.is_time_limit_exceeded() << std::endl;
    std::cout << "wall_time_limit_exceeded? " << task.is_wall_time_limit_exceeded() << std::endl;
    std::cout << "exited?: " << task.exited() << std::endl;
    std::cout << "exit_code: " << task.get_exit_code() << std::endl;
    std::cout << "signaled?: " << task.signaled() << std::endl;
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
