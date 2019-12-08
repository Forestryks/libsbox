/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_TESTING_H
#define LIBSBOX_TESTING_H

#include <libsbox.h>

#include <map>
#include <functional>
#include <cassert>
#include <unistd.h>
#include <cstdarg>
#include <filesystem>

namespace fs = std::filesystem;

class Testing {
public:
    inline static void add_handler(const std::string &name, const std::function<int(std::vector<std::string>)> &handler) {
        handlers[name] = handler;
    }

    [[noreturn]]
    inline static void start(int argc, char *argv[]) {
        assert(argc >= 2);
        assert(handlers.find(argv[1]) != handlers.end());
        std::error_code error;
        current_executable = resolve_path(argv[0]);
        std::vector<std::string> args;
        for (int i = 2; i < argc; ++i) {
            args.emplace_back(argv[i]);
        }
        _exit(handlers[argv[1]](args));
    }

    inline static void safe_run(const std::vector<libsbox::Task *> &tasks) {
        auto error = libsbox::run_together(tasks);
        if (error) {
            std::cerr << "Failed to run tasks: " << error.get() << std::endl;
            exit(1);
        }
    }

    inline static fs::path resolve_path(const fs::path &path) {
        std::error_code error;
        auto res = fs::absolute(path, error);
        if (error) {
            std::cerr << "Failed to resolve path: " << error.message() << std::endl;
            exit(1);
        }
        return res;
    }

    inline static fs::path get_current_executable() {
        return current_executable;
    }

private:
    inline static std::map<std::string, std::function<int(std::vector<std::string>)>> handlers;
    inline static fs::path current_executable;
};

class GenericTarget : public libsbox::Task {
public:
    template<typename ...Args>
    explicit GenericTarget(const Args &...args) {
        set_argv({args...});
        init();
    }

    explicit GenericTarget(const std::vector<std::string> &args) {
        set_argv(args);
        init();
    }

    template<typename ...Args>
    inline static GenericTarget from_local_executable(fs::path executable, const Args &...params) {
        if (!executable.is_absolute()) {
            executable = "." / executable;
        }
        auto res = GenericTarget(executable, params...);
        res.get_binds().emplace_back(executable, Testing::resolve_path(executable));
        return res;
    }

    template<typename ...Args>
    inline static GenericTarget from_current_executable(const Args &...params) {
        std::string filename = Testing::get_current_executable().filename();
        auto res = GenericTarget("./" + filename, params...);
        res.get_binds().emplace_back(filename, Testing::get_current_executable());
        return res;
    }

    void print_stats(std::ostream &stream) {
        stream << "time_usage_ms: " << get_time_usage_ms() << std::endl;
        stream << "time_usage_user_ms: " << get_time_usage_user_ms() << std::endl;
        stream << "time_usage_sys_ms: " << get_time_usage_sys_ms() << std::endl;
        stream << "wall_time_usage_ms: " << get_wall_time_usage_ms() << std::endl;
        stream << "memory_usage_kb: " << get_memory_usage_kb() << std::endl;
        stream << "time_limit_exceeded? " << is_time_limit_exceeded() << std::endl;
        stream << "wall_time_limit_exceeded? " << is_wall_time_limit_exceeded() << std::endl;
        stream << "exited?: " << exited() << std::endl;
        stream << "exit_code: " << get_exit_code() << std::endl;
        stream << "signaled?: " << signaled() << std::endl;
        stream << "term_signal: " << get_term_signal() << std::endl;
        stream << "oom_killed?: " << is_oom_killed() << std::endl;
        stream << "memory_limit_hit?: " << is_memory_limit_hit() << std::endl;
    }

    void assert_exited(int code = -1) {
        assert(!is_time_limit_exceeded());
        assert(!is_wall_time_limit_exceeded());
        assert(exited());
        if (code != -1) assert(get_exit_code() == code);
        assert(!signaled());
        assert(get_term_signal() == -1);
        assert(!is_oom_killed());
        assert(!is_memory_limit_hit());
    }

    void assert_killed(int signal = -1) {
        assert(!exited());
        assert(get_exit_code() == -1);
        assert(signaled());
        if (signal != -1) assert(get_term_signal() == signal);
    }

private:
    void init() {
        set_need_ipc(false);
        set_use_standard_binds(true);
        get_stdin().disable();
        get_stdout().disable();
        get_stderr().disable();
    }
};

#endif //LIBSBOX_TESTING_H
