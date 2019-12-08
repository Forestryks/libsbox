/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_LIBSBOX_H
#define LIBSBOX_LIBSBOX_H

#include <libsbox/error.h>

#include <string>
#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <vector>

namespace libsbox {

static const size_t ARGC_MAX = 128;
static const size_t ENVC_MAX = 128;
static const size_t BINDS_MAX = 10;
static const size_t ARGV_MAX = 4096;
static const size_t ENV_MAX = 4096;

using time_ms_t = int64_t;
using memory_kb_t = int64_t;
using fd_t = int;

static const int unlimited = -1;

class BindRule {
public:
    BindRule(const std::string &inside, const std::string &outside);
    BindRule(const std::string &inside, const std::string &outside, int flags);

    BindRule &allow_write();
    BindRule &allow_dev();
    BindRule &forbid_exec();
    BindRule &allow_suid();
    BindRule &make_optional();

    enum {
        WRITABLE = 1,
        DEV_ALLOWED = 2,
        EXEC_FORBIDDEN = 4,
        SUID_ALLOWED = 8,
        OPTIONAL = 16
    };

    const std::string &get_inside_path() const;
    const std::string &get_outside_path() const;
    int get_flags() const;
private:
    std::string inside_;
    std::string outside_;
    int flags_;

    template<class Writer>
    void serialize_request(Writer &writer) const;
    template<class Value>
    BindRule(const Value &value);

    friend class Task;
};

class Pipe {
public:
    Pipe();

private:
    std::string name_;
    const std::string &get_name() const;

    friend class Stream;
};

class Stream {
public:
    void disable();
    void use_pipe(const Pipe &pipe);
    void use_file(const std::string &filename);

    const std::string &get_filename() const;
protected:
    std::string filename_;
    Stream() = default;

private:
    template<class Writer>
    void serialize_request(Writer &writer) const;
    template<class Value>
    void deserialize_request(const Value &value);

    friend class Task;
};

class StderrStream : public Stream {
public:
    void use_stdout();
};

class Task {
public:
    time_ms_t get_time_limit_ms() const;
    void set_time_limit_ms(time_ms_t time_limit_ms);
    time_ms_t get_wall_time_limit_ms() const;
    void set_wall_time_limit_ms(time_ms_t wall_time_limit_ms);
    memory_kb_t get_memory_limit_kb() const;
    void set_memory_limit_kb(memory_kb_t memory_limit_kb);
    memory_kb_t get_fsize_limit_kb() const;
    void set_fsize_limit_kb(memory_kb_t fsize_limit_kb);
    int32_t get_max_files() const;
    void set_max_files(int32_t max_files);
    int32_t get_max_threads() const;
    void set_max_threads(int32_t max_threads);
    bool get_need_ipc() const;
    void set_need_ipc(bool need_ipc);
    bool get_use_standard_binds() const;
    void set_use_standard_binds(bool use_standard_binds);

    Stream &get_stdin();
    Stream &get_stdout();
    StderrStream &get_stderr();

    const std::vector<std::string> &get_argv() const;
    void set_argv(const std::vector<std::string> &argv);
    std::vector<std::string> &get_env();
    const std::vector<std::string> &get_env() const;
    std::vector<BindRule> &get_binds();
    const std::vector<BindRule> &get_binds() const;

    time_ms_t get_time_usage_ms() const;
    void set_time_usage_ms(time_ms_t time_usage_ms);
    time_ms_t get_time_usage_sys_ms() const;
    void set_time_usage_sys_ms(time_ms_t time_usage_sys_ms);
    time_ms_t get_time_usage_user_ms() const;
    void set_time_usage_user_ms(time_ms_t time_usage_user_ms);
    time_ms_t get_wall_time_usage_ms() const;
    void set_wall_time_usage_ms(time_ms_t wall_time_usage_ms);
    memory_kb_t get_memory_usage_kb() const;
    void set_memory_usage_kb(memory_kb_t memory_usage_kb);
    bool is_time_limit_exceeded() const;
    void set_time_limit_exceeded(bool time_limit_exceeded);
    bool is_wall_time_limit_exceeded() const;
    void set_wall_time_limit_exceeded(bool wall_time_limit_exceeded);
    bool exited() const;
    void set_exited(bool exited);
    int get_exit_code() const;
    void set_exit_code(int exit_code);
    bool signaled() const;
    void set_signaled(bool signaled);
    int get_term_signal() const;
    void set_term_signal(int term_signal);
    bool is_oom_killed() const;
    void set_oom_killed(bool oom_killed);
    bool is_memory_limit_hit() const;
    void set_memory_limit_hit(bool memory_limit_hit);

    template<class Writer>
    void serialize_request(Writer &writer) const;

    template<class Value>
    void deserialize_request(const Value &value);

    template<class Writer>
    void serialize_response(Writer &writer) const;

    template<class Value>
    Error deserialize_response(const Value &value);
private:
    // parameters
    time_ms_t time_limit_ms_ = -1;
    time_ms_t wall_time_limit_ms_ = -1;
    memory_kb_t memory_limit_kb_ = -1;
    memory_kb_t fsize_limit_kb_ = -1;
    int32_t max_files_ = 16;
    int32_t max_threads_ = 1;
    bool need_ipc_ = false;
    bool use_standard_binds_ = true;

    Stream stdin_;
    Stream stdout_;
    StderrStream stderr_;
    std::vector<std::string> argv_;
    std::vector<std::string> env_;
    std::vector<BindRule> binds_;

    // results
    time_ms_t time_usage_ms_ = 0;
    time_ms_t time_usage_sys_ms_ = 0;
    time_ms_t time_usage_user_ms_ = 0;
    time_ms_t wall_time_usage_ms_ = 0;
    memory_kb_t memory_usage_kb_ = 0;

    bool time_limit_exceeded_ = false;
    bool wall_time_limit_exceeded_ = false;
    bool exited_ = false;
    int exit_code_ = -1;
    bool signaled_ = false;
    int term_signal_ = -1;
    bool oom_killed_ = false;
    bool memory_limit_hit_ = false;
};

Error run_together(const std::vector<Task *> &tasks, const std::string &socket_path = "/etc/libsboxd/socket");

} // namespace libsbox

#endif //LIBSBOX_LIBSBOX_H
