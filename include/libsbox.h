/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_LIBSBOX_H
#define LIBSBOX_LIBSBOX_H

#include <string>
#include <libsbox/external/json.hpp>
#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace libsbox {

using time_ms_t = int64_t;
using memory_kb_t = int64_t;
using fd_t = int;

class Bind {
public:
    Bind(std::string inside, std::string outside) : inside_(std::move(inside)), outside_(std::move(outside)) {}

    Bind &allow_write() {
        flags_ |= READWRITE;
        return (*this);
    }

    Bind &allow_dev() {
        flags_ |= DEV;
        return (*this);
    }

    Bind &forbid_exec() {
        flags_ |= NOEXEC;
        return (*this);
    }

    Bind &allow_suid() {
        flags_ |= SUID;
        return (*this);
    }

    Bind &make_optional() {
        flags_ |= OPT;
        return (*this);
    }

private:
    std::string inside_;
    std::string outside_;
    int flags_ = 0;

    enum {
        READWRITE = 1,
        DEV = 2,
        NOEXEC = 4,
        SUID = 8,
        OPT = 16
    };

    friend void to_json(nlohmann::json &json, const Bind &bind);
};

inline void to_json(nlohmann::json &json, const Bind &bind) {
    json = nlohmann::json(
        {
            {"inside", bind.inside_},
            {"outside", bind.outside_},
            {"flags", bind.flags_}
        }
    );
}

class Pipe {
public:
    Pipe() {
        static int32_t counter = 0;
        name_ = "pipe-" + std::to_string(counter++);
    }

private:
    const std::string &get_name() const {
        return name_;
    }

    std::string name_;
    friend class Stream;
};

class Stream {
public:
    void disable() {
        filename_.clear();
    }

    void use_pipe(const Pipe &pipe) {
        filename_ = "@" + pipe.get_name();
    }

    void use_file(const std::string &filename) {
        filename_ = filename;
    }

protected:
    std::string filename_;
    Stream() = default;
private:
    friend class Task;
};

class StderrStream : public Stream {
public:
    void use_stdout() {
        filename_ = "@stdout";
    }
};

class Task {
public:
    void set_time_limit_ms(time_ms_t time_limit_ms);
    void set_wall_time_limit_ms(time_ms_t wall_time_limit_ms);
    void set_memory_limit_kb(memory_kb_t memory_limit_kb);
    void set_fsize_limit_kb(memory_kb_t fsize_limit_kb);
    void set_max_files(int32_t max_files);
    void set_max_threads(int32_t max_threads);
    void set_need_ipc(bool need_ipc);
    void set_use_standard_binds(bool use_standard_binds);

    Stream &get_stdin();
    Stream &get_stdout();
    StderrStream &get_stderr();

    void set_argv(const std::vector<std::string> &argv);
    void set_env(const std::vector<std::string> &env);
    void add_bind(const Bind &bind);
    void clear_binds();

    time_ms_t get_time_usage_ms() const;
    time_ms_t get_time_usage_sys_ms() const;
    time_ms_t get_time_usage_user_ms() const;
    time_ms_t get_wall_time_usage_ms() const;
    memory_kb_t get_memory_usage_kb() const;
    bool is_time_limit_exceeded() const;
    bool is_wall_time_limit_exceeded() const;
    bool exited() const;
    int get_exit_code() const;
    bool is_signaled() const;
    int get_term_signal() const;
    bool is_oom_killed() const;
    bool is_memory_limit_hit() const;
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
    std::vector<Bind> binds_;

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

    nlohmann::json serialize_parameters();
    void deserialize_results(const nlohmann::json &json_results);

    friend int run(const std::vector<Task *> &, const std::string &);
};

void Task::set_time_limit_ms(time_ms_t time_limit_ms) {
    time_limit_ms_ = time_limit_ms;
}

void Task::set_wall_time_limit_ms(time_ms_t wall_time_limit_ms) {
    wall_time_limit_ms_ = wall_time_limit_ms;
}

void Task::set_memory_limit_kb(memory_kb_t memory_limit_kb) {
    memory_limit_kb_ = memory_limit_kb;
}

void Task::set_fsize_limit_kb(memory_kb_t fsize_limit_kb) {
    fsize_limit_kb_ = fsize_limit_kb;
}

void Task::set_max_files(int32_t max_files) {
    max_files_ = max_files;
}

void Task::set_max_threads(int32_t max_threads) {
    max_threads_ = max_threads;
}

void Task::set_need_ipc(bool need_ipc) {
    need_ipc_ = need_ipc;
}

void Task::set_use_standard_binds(bool use_standard_binds) {
    use_standard_binds_ = use_standard_binds;
}

Stream &Task::get_stdin() {
    return stdin_;
}

Stream &Task::get_stdout() {
    return stdout_;
}

StderrStream &Task::get_stderr() {
    return stderr_;
}

void Task::set_argv(const std::vector<std::string> &argv) {
    argv_ = argv;
}

void Task::set_env(const std::vector<std::string> &env) {
    env_ = env;
}

void Task::add_bind(const Bind &bind) {
    binds_.push_back(bind);
}

void Task::clear_binds() {
    binds_.clear();
}

time_ms_t Task::get_time_usage_ms() const {
    return time_usage_ms_;
}

time_ms_t Task::get_time_usage_sys_ms() const {
    return time_usage_sys_ms_;
}

time_ms_t Task::get_time_usage_user_ms() const {
    return time_usage_user_ms_;
}

time_ms_t Task::get_wall_time_usage_ms() const {
    return wall_time_usage_ms_;
}

memory_kb_t Task::get_memory_usage_kb() const {
    return memory_usage_kb_;
}

bool Task::is_time_limit_exceeded() const {
    return time_limit_exceeded_;
}

bool Task::is_wall_time_limit_exceeded() const {
    return wall_time_limit_exceeded_;
}

bool Task::exited() const {
    return exited_;
}

int Task::get_exit_code() const {
    return exit_code_;
}

bool Task::is_signaled() const {
    return signaled_;
}

int Task::get_term_signal() const {
    return term_signal_;
}

bool Task::is_oom_killed() const {
    return oom_killed_;
}

bool Task::is_memory_limit_hit() const {
    return memory_limit_hit_;
}

nlohmann::json Task::serialize_parameters() {
    return {
        {"time_limit_ms", time_limit_ms_},
        {"wall_time_limit_ms", wall_time_limit_ms_},
        {"memory_limit_kb", memory_limit_kb_},
        {"fsize_limit_kb", fsize_limit_kb_},
        {"max_files", max_files_},
        {"max_threads", max_threads_},
        {"ipc", need_ipc_},
        {"standard_binds", use_standard_binds_},
        {"stdin", stdin_.filename_},
        {"stdout", stdout_.filename_},
        {"stderr", stderr_.filename_},
        {"argv", argv_},
        {"env", env_},
        {"binds", binds_}
    };
}

void Task::deserialize_results(const nlohmann::json &json_results) {
    time_usage_ms_ = json_results.at("time_usage_ms");
    time_usage_sys_ms_ = json_results.at("time_usage_sys_ms");
    time_usage_user_ms_ = json_results.at("time_usage_user_ms");
    wall_time_usage_ms_ = json_results.at("wall_time_usage_ms");
    memory_usage_kb_ = json_results.at("memory_usage_kb");
    time_limit_exceeded_ = json_results.at("time_limit_exceeded");
    wall_time_limit_exceeded_ = json_results.at("wall_time_limit_exceeded");
    exited_ = json_results.at("exited");
    exit_code_ = json_results.at("exit_code");
    signaled_ = json_results.at("signaled");
    term_signal_ = json_results.at("term_signal");
    oom_killed_ = json_results.at("oom_killed");
    memory_limit_hit_ = json_results.at("memory_limit_hit");
}

inline int run(const std::vector<Task *> &tasks, const std::string &socket_path = "/etc/libsboxd/socket") {
    nlohmann::json json_tasks = nlohmann::json::array();
    for (auto task : tasks) {
        json_tasks.push_back(task->serialize_parameters());
    }

    std::string msg = nlohmann::json::object({{"tasks", json_tasks}}).dump();

    fd_t socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        return socket_fd;
    }

    std::shared_ptr<fd_t> ptr(&socket_fd, [](const fd_t *fd) { close(*fd); });

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path.c_str(), std::min(sizeof(addr.sun_path) - 1, socket_path.size()));

    int status = connect(socket_fd, reinterpret_cast<const struct sockaddr *>(&addr), sizeof(struct sockaddr_un));
    if (status != 0) {
        return status;
    }

    int cnt = send(socket_fd, msg.c_str(), msg.size() + 1, 0);
    if (cnt < 0 || static_cast<size_t>(cnt) != msg.size() + 1) {
        return cnt;
    }

    std::string res;
    char buf[1024];
    while (true) {
        cnt = recv(socket_fd, buf, 1023, 0);
        if (cnt < 0) {
            return cnt;
        }
        if (cnt == 0) {
            break;
        }
        buf[cnt] = 0;
        res += buf;
    }

    nlohmann::json json_result = nlohmann::json::parse(res);

    for (size_t i = 0; i < tasks.size(); ++i) {
        tasks[i]->deserialize_results(json_result.at("tasks").at(i));
    }

    return 0;
}

} // namespace libsbox

#endif //LIBSBOX_LIBSBOX_H
