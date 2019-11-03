/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include "container.h"
#include "daemon.h"
#include "worker.h"
#include "bind.h"
#include "config.h"
#include "utils.h"
#include "signals.h"

#include <unistd.h>
#include <signal.h>
#include <sys/prctl.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <fcntl.h>
#include <grp.h>
#include <dirent.h>

Container::Container(int id, bool persistent) : id_(id), persistent_(persistent) {}

void Container::die(const std::string &error) {
    if (slave_pid_ == 0) {
        task_data_->error = true;
        Daemon::get().report_error("[slave] " + error);
    } else {
        Daemon::get().report_error("[container] " + error);
        if (cpuacct_controller_ != nullptr) cpuacct_controller_->die();
        if (memory_controller_ != nullptr) memory_controller_->die();
    }

    _exit(1);
}

void Container::terminate() {
    die("Container should never be terminated");
}

int Container::get_id() {
    return id_;
}

pid_t Container::get_pid() {
    return pid_;
}

SharedBarrier *Container::get_barrier() {
    return &barrier_;
}

void Container::get_task_from_json(const nlohmann::json &json_task) {
    task_data_->time_limit_ms = json_task["time_limit_ms"];
    task_data_->wall_time_limit_ms = json_task["wall_time_limit_ms"];
    task_data_->memory_limit_kb = json_task["memory_limit_kb"];
    task_data_->fsize_limit_kb = json_task["fsize_limit_kb"];
    task_data_->max_files = json_task["max_files"];
    task_data_->max_threads = json_task["max_threads"];
    task_data_->ipc = json_task["ipc"];

    task_data_->stdin_desc.fd = -1;
    task_data_->stdin_desc.filename = "";
    if (!json_task["stdin"].is_null() && !json_task["stdin"].empty()) {
        std::string stdin_filename = json_task["stdin"];
        if (stdin_filename[0] == '@') {
            const auto &pipe = Worker::get().get_pipe(stdin_filename.substr(1));
            task_data_->stdin_desc.fd = pipe.first;
        } else {
            task_data_->stdin_desc.filename = stdin_filename;
        }
    }

    task_data_->stdout_desc.fd = -1;
    task_data_->stdout_desc.filename = "";
    if (!json_task["stdout"].is_null() && !json_task["stdout"].empty()) {
        std::string stdout_filename = json_task["stdout"];
        if (stdout_filename[0] == '@') {
            const auto &pipe = Worker::get().get_pipe(stdout_filename.substr(1));
            task_data_->stdout_desc.fd = pipe.second;
        } else {
            task_data_->stdout_desc.filename = stdout_filename;
        }
    }

    task_data_->stderr_desc.fd = -1;
    task_data_->stderr_desc.filename = "";
    if (!json_task["stderr"].is_null() && !json_task["stderr"].empty()) {
        std::string stderr_filename = json_task["stderr"];
        if (stderr_filename[0] == '@') {
            std::string pipe_name = stderr_filename.substr(1);
            if (pipe_name == "stdout") {
                task_data_->stderr_desc.fd = 1;
            } else {
                const auto &pipe = Worker::get().get_pipe(pipe_name);
                task_data_->stderr_desc.fd = pipe.second;
            }
        } else {
            task_data_->stderr_desc.filename = stderr_filename;
        }
    }

    int argv_size = json_task.at("argv").size();
    task_data_->argv.clear();
    for (int i = 0; i < argv_size; ++i) {
        task_data_->argv.add(json_task.at("argv").at(i));
    }

    // ENV to do?
    task_data_->env.clear();

    task_data_->binds.clear();
    for (const auto &bind : json_task.at("binds")) {
        std::string inside = bind["inside"];
        std::string outside = bind["outside"];
        int flags = bind["flags"];
        task_data_->binds.emplace_back(inside, outside, flags);
    }

    task_data_->time_usage_ms = -1;
    task_data_->time_usage_sys_ms = -1;
    task_data_->time_usage_user_ms = -1;
    task_data_->wall_time_usage_ms = -1;
    task_data_->memory_usage_kb = -1;

    task_data_->time_limit_exceeded = false;
    task_data_->wall_time_limit_exceeded = false;
    task_data_->exited = false;
    task_data_->exit_code = -1;
    task_data_->signaled = false;
    task_data_->term_signal = -1;
    task_data_->oom_killed = false;

    task_data_->error = false;
}

nlohmann::json Container::results_to_json() {
    return nlohmann::json::object(
        {
            {"time_usage_ms", task_data_->time_usage_ms},
            {"time_usage_sys_ms", task_data_->time_usage_sys_ms},
            {"time_usage_user_ms", task_data_->time_usage_user_ms},
            {"wall_time_usage_ms", task_data_->wall_time_usage_ms},
            {"memory_usage_kb", task_data_->memory_usage_kb},
            {"time_limit_exceeded", task_data_->time_limit_exceeded},
            {"wall_time_limit_exceeded", task_data_->wall_time_limit_exceeded},
            {"exited", task_data_->exited},
            {"exit_code", task_data_->exit_code},
            {"signaled", task_data_->signaled},
            {"term_signal", task_data_->term_signal},
            {"oom_killed", task_data_->oom_killed}
        }
    );
}

int Container::clone_callback(void *ptr) {
    ((Container *) ptr)->serve();
    return 0;
}

pid_t Container::start() {
    const int clone_stack_size = 8 * 1024 * 1024;
    char *clone_stack = new char[clone_stack_size];
    // SIGCHLD - send SIGCHLD on exit
    // CLONE_FILES - share file descriptors table
    // CLONE_PARENT - make daemon parent of container, so if container exits, daemon will be notified
    // CLONE_NEWIPC - create new ipc namespace to prevent any forms of communication
    // CLONE_NEWNET - create new network namespace to block network
    // CLONE_NEWNS - create new mount namespace (used for safety reasons)
    // CLONE_NEWPID - create new pid namespace to hide other processes
    pid_ = clone(
        clone_callback,
        clone_stack + clone_stack_size,
        SIGCHLD | CLONE_FILES | CLONE_NEWIPC | CLONE_NEWNET | CLONE_NEWNS | CLONE_NEWPID,
        this
    );
    delete[] clone_stack;

    return pid_;
}

void Container::serve() {
    ContextManager::set(this);
    prepare();

    while (true) {
        // Wait for task
        barrier_.wait();

        std::vector<Bind> binds;

        for (size_t i = 0; i < task_data_->binds.size(); ++i) {
            binds.emplace_back(&task_data_->binds[i]);
            binds[i].mount(root_, work_dir_);
        }

        cpuacct_controller_ = new CgroupController("cpuacct", std::to_string(id_));
        memory_controller_ = new CgroupController("memory", std::to_string(id_));

        slave_pid_ = fork();
        if (slave_pid_ < 0) {
            die(format("fork() failed: %m"));
        }
        if (slave_pid_ == 0) {
            slave();
        }

        // Run started
        Worker::get().get_run_start_barrier()->wait();

        wait_for_slave();

        for (auto &bind : binds) {
            bind.umount(root_, work_dir_);
        }

        delete cpuacct_controller_;
        cpuacct_controller_ = nullptr;
        delete memory_controller_;
        memory_controller_ = nullptr;

        // Results ready
        barrier_.wait();

        if (!persistent_) break;
    }

    _exit(0);
}

void Container::prepare() {
    root_ = Config::get().get_box_dir();

    if (prctl(PR_SET_PDEATHSIG, SIGKILL) != 0) {
        die(format("Cannot set parent death signal: %m"));
    }
    // To prevent race condition
    if (getppid() == 0) {
        raise(SIGKILL);
    }

    reset_signals();

    prepare_root();
    if (persistent_) {
        disable_ipcs();
    }
}

void Container::prepare_root() {
    if (mount("none", "/", "none", MS_REC | MS_PRIVATE, nullptr) != 0) {
        die(format("Cannot privatize mounts: %m"));
    }

    std::error_code error;
    fs::create_directories(root_, error);
    if (error) {
        die(format("Cannot create root directory (%s): %m", root_.c_str()));
    }
    if (mount("none", root_.c_str(), "tmpfs", 0, "mode=755") != 0) {
        die(format("Cannot mount root tmpfs: %m"));
    }

    fs::path proc_dir = root_ / "proc";
    fs::create_directories(proc_dir, error);
    if (error) {
        die(format("Cannot create '/proc' dir: %m"));
    }
    if (mount("none", proc_dir.c_str(), "proc", 0, "") < 0) {
        die(format("Cannot mount proc filesystem: %m"));
    }
    if (mount("none", proc_dir.c_str(), "proc", MS_REMOUNT, "hidepid=2") < 0) {
        die(format("Cannot remount proc filesystem: %m"));
    }

    work_dir_ = root_ / "work";
    fs::create_directories(work_dir_, error);
    if (error) {
        die(format("Cannot create '/work' dir: %m"));
    }

    for (auto &it : Bind::get_standard_binds()) {
        it.mount(root_, work_dir_);
    }
}

void Container::disable_ipcs() {
    write_file("/proc/sys/kernel/msgmni", "0");
    write_file("/proc/sys/kernel/shmmni", "0");
    write_file("/proc/sys/kernel/sem", "0 0 0 0");
}

void Container::wait_for_slave() {
    start_timer(Config::get().get_timer_interval_ms());
    reset_wall_clock();

    while (true) {
        int status;
        pid_t pid = waitpid(slave_pid_, &status, 0);
        if (pid != slave_pid_) {
            if (!timer_interrupt) {
                die(format("waitpid() failed: %m"));
            }

            timer_interrupt = false;

            if (task_data_->time_limit_ms != -1 && get_time_usage_ms() > task_data_->time_limit_ms) {
                kill_all();
                break;
            }

            if (task_data_->wall_time_usage_ms != -1 && get_wall_clock() > task_data_->wall_time_limit_ms) {
                kill_all();
                break;
            }

            continue;
        }

        if (WIFEXITED(status)) {
            task_data_->exited = true;
            task_data_->exit_code = WEXITSTATUS(status);
        }
        if (WIFSIGNALED(status)) {
            task_data_->signaled = true;
            task_data_->term_signal = WTERMSIG(status);
        }

        break;
    }

    stop_timer();

    task_data_->time_usage_ms = get_time_usage_ms();
    task_data_->time_usage_sys_ms = get_time_usage_sys_ms();
    task_data_->time_usage_user_ms = get_time_usage_user_ms();
    task_data_->wall_time_usage_ms = get_wall_clock();
    task_data_->memory_usage_kb = get_memory_usage_kb();
    task_data_->oom_killed = is_oom_killed();
    if (task_data_->time_limit_ms != -1) {
        task_data_->time_limit_exceeded = (task_data_->time_usage_ms > task_data_->time_limit_ms);
    }
    if (task_data_->wall_time_limit_ms != -1) {
        task_data_->wall_time_limit_exceeded = (task_data_->wall_time_usage_ms > task_data_->wall_time_limit_ms);
    }

    if (task_data_->error) {
        die("Slave exited with error");
    }
}

void Container::kill_all() {
    if (kill(-1, SIGKILL) != 0) {
        die(format("Failed to kill all processes in box: %m"));
    }
}

void Container::reset_wall_clock() {
    gettimeofday(&run_start_, nullptr);
}

long Container::get_wall_clock() {
    struct timeval now = {}, seg = {};
    gettimeofday(&now, nullptr);
    timersub(&now, &run_start_, &seg);
    return seg.tv_sec * 1000 + seg.tv_usec / 1000;
}

long Container::get_time_usage_ms() {
    std::string data = cpuacct_controller_->read("cpuacct.usage");
    return stoll(data) / 1000000;
}

long Container::get_time_usage_sys_ms() {
    std::string data = cpuacct_controller_->read("cpuacct.usage_sys");
    return stoll(data) / 1000000;
}

long Container::get_time_usage_user_ms() {
    std::string data = cpuacct_controller_->read("cpuacct.usage_user");
    return stoll(data) / 1000000;
}

long Container::get_memory_usage_kb() {
    long long max_usage = stoll(memory_controller_->read("memory.max_usage_in_bytes"));
    long long cur_usage = stoll(memory_controller_->read("memory.usage_in_bytes"));
    return std::max(max_usage, cur_usage) / 1024;
}

bool Container::is_oom_killed() {
    std::string data = memory_controller_->read("memory.oom_control");
    std::stringstream sstream(memory_controller_->read("memory.oom_control"));
    std::string name, val;
    while (sstream >> name >> val) {
        if (name == "oom_kill") {
            return (val != "0");
        }
    }
    die("Can't find oom_kill field in memory.oom_control");
    _exit(-1); // we should not get here
}

void Container::freopen_fds() {
    if (task_data_->stdin_desc.filename.empty()) {
        task_data_->stdin_desc.fd = open(task_data_->stdin_desc.filename.c_str(), O_RDONLY);
        if (task_data_->stdin_desc.fd < 0) {
            die(format("Cannot open '%s': %m", task_data_->stdin_desc.filename.c_str()));
        }
    }

    if (!task_data_->stdout_desc.filename.empty()) {
        task_data_->stdout_desc.fd =
            open(task_data_->stdout_desc.filename.c_str(), O_WRONLY | O_TRUNC);
        if (task_data_->stdout_desc.fd < 0) {
            die(format("Cannot open '%s': %m", task_data_->stdout_desc.filename.c_str()));
        }
    }

    if (!task_data_->stderr_desc.filename.empty()) {
        task_data_->stderr_desc.fd = open(task_data_->stderr_desc.filename.c_str(), O_WRONLY | O_TRUNC);
        if (task_data_->stderr_desc.fd < 0) {
            die(format("Cannot open '%s': %m", task_data_->stderr_desc.filename.c_str()));
        }
    }
}

void Container::dup2_fds() {
    if (close(0) != 0) {
        die(format("Cannot close stdin: %m"));
    }
    if (close(1) != 0) {
        die(format("Cannot close stdout: %m"));
    }
    if (close(2) != 0) {
        die(format("Cannot close stderr: %m"));
    }

    if (task_data_->stdin_desc.fd != -1) {
        if (dup2(task_data_->stdin_desc.fd, 0) != 0) {
            die(format("Cannot dup2 stdin: %m"));
        }
    }

    if (task_data_->stdout_desc.fd != -1) {
        if (dup2(task_data_->stdout_desc.fd, 1) != 1) {
            die(format("Cannot dup2 stdout: %m"));
        }
    }

    if (task_data_->stderr_desc.fd != -1) {
        if (dup2(task_data_->stderr_desc.fd, 2) != 2) {
            die(format("Cannot dup2 stderr: %m"));
        }
    }
}

void Container::close_all_fds() {
    DIR *dir = opendir("/proc/self/fd");
    if (dir == nullptr) {
        die(format("Cannot open /proc/self/fd: %m"));
    }
    int dir_fd = dirfd(dir);
    if (dir_fd < 0) {
        die(format("Cannot get fd from DIR*: %m"));
    }

    struct dirent *dentry;
    while ((dentry = readdir(dir))) {
        char *end;
        long fd = strtol(dentry->d_name, &end, 10);
        if (*end) continue;

        if ((fd >= 0 && fd <= 2) || fd == dir_fd || fd == exec_fd_ || fd == Daemon::get().get_error_pipe_fd())
            continue;
        if (close((int) fd) != 0) {
            die(format("Cannot close fd %ld: %m", fd));
        }
    }

    if (closedir(dir) != 0) {
        die(format("Cannot close /proc/: %m"));
    }
}

void Container::set_rlimit_ext(const char *res_name, int res, rlim_t limit) {
    struct rlimit rlim = {limit, limit};
    if (setrlimit(res, &rlim) != 0) {
        die(format("Cannot set %s: %m", res_name));
    }
}

#define set_rlimit(res, limit) set_rlimit_ext(#res, res, limit)

void Container::setup_rlimits() {
    if (task_data_->fsize_limit_kb != -1) {
        set_rlimit(RLIMIT_FSIZE, task_data_->fsize_limit_kb);
    }

    set_rlimit(RLIMIT_STACK, RLIM_INFINITY);
    set_rlimit(RLIMIT_NOFILE, (task_data_->max_files == -1 ? RLIM_INFINITY : task_data_->max_files));
    set_rlimit(RLIMIT_NPROC, (task_data_->max_threads == -1 ? RLIM_INFINITY : task_data_->max_threads));
    set_rlimit(RLIMIT_MEMLOCK, 0);
    set_rlimit(RLIMIT_MSGQUEUE, 0);
}

#undef set_rlimit

void Container::setup_credentials() {
    if (setresgid(id_, id_, id_) != 0) {
        die(format("Setting process gid to %d failed: %m", id_));
    }
    if (setgroups(0, nullptr) != 0) {
        die(format("Removing process from all groups failed: %m"));
    }
    if (setresuid(id_, id_, id_) != 0) {
        die(format("Setting process uid to %d failed: %m", id_));
    }
    if (setpgrp() != 0) {
        die(format("Setting process group id failed: %m"));
    }
}

void Container::slave() {
    fs::path executable = task_data_->argv[0];
    if (!executable.is_absolute()) {
        die(format("%s is not absolute path", executable.c_str()));
    }
    exec_fd_ = open(executable.c_str(), O_RDONLY | O_CLOEXEC);
    if (exec_fd_ < 0) {
        die(format("Cannot open target executable '%s': %m", executable.c_str()));
    }

    if (chdir(work_dir_.c_str()) != 0) {
        die(format("chdir() failed: %m"));
    }

    if (setresuid(id_, 0, 0) != 0) {
        die(format("Cannot set ruid to %d: %m", id_));
    }

    freopen_fds();
    dup2_fds();
    close_all_fds();

    memory_controller_->enter();
    cpuacct_controller_->enter();
    setup_rlimits();

    if (chroot("..") != 0) {
        die(format("chroot() failed: %m"));
    }

    setup_credentials();

    fexecve(exec_fd_, task_data_->argv.get(), task_data_->env.get());
    die(format("Failed to exec target: %m"));
    _exit(-1); // we should not get here
}
