/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include <libsbox/worker.h>
#include <libsbox/proxy.h>
#include <libsbox/signals.h>
#include <libsbox/config.h>

#include <syslog.h>
#include <sched.h>
#include <sys/signal.h>
#include <iostream>
#include <sys/mount.h>
#include <wait.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <grp.h>

Worker *Worker::worker_;

Worker::Worker(int uid) : uid_(uid) {
    shared_memory_ = new SharedMemory(sizeof(target_desc));
    target_desc_ = (target_desc *) shared_memory_->get();
}

Worker::~Worker() {
    delete shared_memory_;
}

Worker &Worker::get() {
    return *worker_;
}

int clone_callback(void *ptr) {
    return ((Worker *) ptr)->serve();
}

void Worker::start() {
    const int clone_stack_size = 8 * 1024 * 1024;
    char *clone_stack = new char[clone_stack_size];
    // SIGCHLD - send SIGCHLD on exit
    // CLONE_FILES - share file descriptors table
    // CLONE_PARENT - make daemon parent of worker, so if worker exits, daemon will be notified
    // CLONE_NEWIPC - create new ipc namespace to prevent any forms of communication
    // CLONE_NEWNET - create new network namespace to block network
    // CLONE_NEWNS - create new mount namespace (used for safety reasons)
    // CLONE_NEWPID - create new pid namespace to hide other processes
    pid_t pid = clone(
        clone_callback,
        clone_stack + clone_stack_size,
        SIGCHLD | CLONE_FILES | CLONE_PARENT | CLONE_NEWIPC | CLONE_NEWNET | CLONE_NEWNS | CLONE_NEWPID,
        this
    );
    delete[] clone_stack;

    if (pid < 0) {
        Proxy::get().die(format("Cannot spawn worker (clone failed): %m"));
    }
}

[[noreturn]]
void Worker::die(const std::string &error) {
    if (slave_pid_ == 0) {
        syslog(LOG_ERR, "%s", ("[slave] " + error).c_str());
        target_desc_->error = true;
    } else {
        if (cpuacct_controller_ != nullptr) cpuacct_controller_->die();
        if (memory_controller_ != nullptr) memory_controller_->die();
        syslog(LOG_ERR, "%s", ("[proxy] " + error).c_str());
    }

    exit(1);
}

int Worker::serve() {
    Context::set_context(this);
    worker_ = this;
    prepare();

    while (true) {
        desc_write_barrier_.wait(1);

        std::vector<Bind> binds;

        // TODO: make this persistent
        for (int i = 0; i < target_desc_->bind_count; ++i) {
            binds.emplace_back(&target_desc_->binds[i]);
            binds[i].mount(root_, work_dir_);
        }

        cpuacct_controller_ = new CgroupController("cpuacct", std::to_string(uid_));
        memory_controller_ = new CgroupController("memory", std::to_string(uid_));

        slave_pid_ = fork();
        if (slave_pid_ < 0) {
            die(format("fork() failed: %m"));
        }
        if (slave_pid_ == 0) {
            slave();
        }

        Proxy::get().get_start_barrier().sub();

        wait_for_target();

        for (int i = 0; i < target_desc_->bind_count; ++i) {
            binds[i].umount(root_, work_dir_);
        }

        delete cpuacct_controller_;
        delete memory_controller_;
        cpuacct_controller_ = nullptr;
        memory_controller_ = nullptr;

        Proxy::get().get_end_barrier().sub();
    }
}

void Worker::prepare() {
//    reset_signals();
    root_ = Config::get().get_box_dir();
    prepare_root();
    prepare_ipcs();
}

void Worker::prepare_root() {
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

void Worker::prepare_ipcs() {
    write_file("/proc/sys/kernel/msgmni", "0");
    write_file("/proc/sys/kernel/shmmni", "0");
    write_file("/proc/sys/kernel/sem", "0 0 0 0");
}

void Worker::kill_all() {
    if (kill(-1, SIGKILL) != 0) {
        die(format("Failed to kill all processes in box: %m"));
    }
}

SharedBarrier *Worker::get_desc_write_barrier() {
    return &desc_write_barrier_;
}

target_desc *Worker::get_target_desc() const {
    return target_desc_;
}

void Worker::freopen_fds() {
    if (target_desc_->stdin_desc.filename[0] != 0) {
        target_desc_->stdin_desc.fd = open(target_desc_->stdin_desc.filename, O_RDONLY);
        if (target_desc_->stdin_desc.fd < 0) {
            die(format("Cannot open '%s': %m", target_desc_->stdin_desc.filename));
        }
    }

    if (target_desc_->stdout_desc.filename[0] != 0) {
        target_desc_->stdout_desc.fd =
            open(target_desc_->stdout_desc.filename, O_WRONLY | O_TRUNC); // TODO: remove O_CREAT?
        if (target_desc_->stdout_desc.fd < 0) {
            die(format("Cannot open '%s': %m", target_desc_->stdout_desc.filename));
        }
    }

    if (target_desc_->stderr_desc.filename[0] != 0) {
        target_desc_->stderr_desc.fd = open(target_desc_->stderr_desc.filename, O_WRONLY | O_TRUNC);
        if (target_desc_->stderr_desc.fd < 0) {
            die(format("Cannot open '%s': %m", target_desc_->stderr_desc.filename));
        }
    }
}

void Worker::dup2_fds() {
    if (close(0) != 0) {
        die(format("Cannot close stdin: %m"));
    }
    if (close(1) != 0) {
        die(format("Cannot close stdout: %m"));
    }
    if (close(2) != 0) {
        die(format("Cannot close stderr: %m"));
    }

    if (target_desc_->stdin_desc.fd != -1) {
        if (dup2(target_desc_->stdin_desc.fd, 0) != 0) {
            die(format("Cannot dup2 stdin: %m"));
        }
    }

    if (target_desc_->stdout_desc.fd != -1) {
        if (dup2(target_desc_->stdout_desc.fd, 1) != 1) {
            die(format("Cannot dup2 stdout: %m"));
        }
    }

    if (target_desc_->stderr_desc.fd != -1) {
        if (dup2(target_desc_->stderr_desc.fd, 2) != 2) {
            die(format("Cannot dup2 stderr: %m"));
        }
    }
}

void Worker::close_all_fds() {
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

        if ((fd >= 0 && fd <= 2) || fd == dir_fd || fd == exec_fd_)
            continue;
        if (close((int) fd) != 0) {
            die(format("Cannot close fd %ld: %m", fd));
        }
    }

    if (closedir(dir) != 0) {
        die(format("Cannot close /proc/: %m"));
    }
}

void Worker::set_rlimit_ext(const char *res_name, int res, rlim_t limit) {
    struct rlimit rlim = {limit, limit};
    if (setrlimit(res, &rlim) != 0) {
        die(format("Cannot set %s: %m", res_name));
    }
}

#define set_rlimit(res, limit) set_rlimit_ext(#res, res, limit)

void Worker::setup_rlimits() {
    if (target_desc_->fsize_limit_kb != -1) {
        set_rlimit(RLIMIT_FSIZE, target_desc_->fsize_limit_kb);
    }

    set_rlimit(RLIMIT_STACK, RLIM_INFINITY);
    set_rlimit(RLIMIT_NOFILE, (target_desc_->max_files == -1 ? RLIM_INFINITY : target_desc_->max_files));
    set_rlimit(RLIMIT_NPROC, (target_desc_->max_threads == -1 ? RLIM_INFINITY : target_desc_->max_threads));
    set_rlimit(RLIMIT_MEMLOCK, 0);
    set_rlimit(RLIMIT_MSGQUEUE, 0);
}

#undef set_rlimit

void Worker::setup_credentials() {
    if (setresgid(uid_, uid_, uid_) != 0) {
        die(format("Setting process gid to %d failed: %m", uid_));
    }
    if (setgroups(0, nullptr) != 0) {
        die(format("Removing process from all groups failed: %m"));
    }
    if (setresuid(uid_, uid_, uid_) != 0) {
        die(format("Setting process uid to %d failed: %m", uid_));
    }
    if (setpgrp() != 0) {
        die(format("Setting process group id failed: %m"));
    }
}

[[noreturn]]
void Worker::slave() {
    fs::path executable = target_desc_->argv[0];
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

    if (setresuid(uid_, 0, 0) != 0) {
        die(format("Cannot set ruid to %d: %m", uid_));
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

    fexecve(exec_fd_, target_desc_->argv, target_desc_->env);
    die(format("Failed to exec target: %m"));
    exit(-1); // we should not get here
}

void Worker::wait_for_target() {
    start_timer(Config::get().get_timer_interval_ms());
    reset_wall_clock();

    while (true) {
        int status;
        pid_t pid = waitpid(slave_pid_, &status, 0);
        if (pid < 0) {
            if (errno != EINTR || interrupt_signal != SIGALRM) {
                die(format("waitpid() failed: %m"));
            }

            if (target_desc_->time_limit_ms != -1 && get_time_usage_ms() > target_desc_->time_limit_ms) {
                kill_all();
                break;
            }

            if (target_desc_->wall_time_usage_ms != -1 && get_wall_clock() > target_desc_->wall_time_limit_ms) {
                kill_all();
                break;
            }

            continue;
        }

        if (WIFEXITED(status)) {
            target_desc_->exited = true;
            target_desc_->exit_code = WEXITSTATUS(status);
        }
        if (WIFSIGNALED(status)) {
            target_desc_->signaled = true;
            target_desc_->term_signal = WTERMSIG(status);
        }

        break;
    }

    target_desc_->time_usage_ms = get_time_usage_ms();
    target_desc_->time_usage_sys_ms = get_time_usage_sys_ms();
    target_desc_->time_usage_user_ms = get_time_usage_user_ms();
    target_desc_->wall_time_usage_ms = get_wall_clock();
    target_desc_->memory_usage_kb = get_memory_usage_kb();
    target_desc_->oom_killed = is_oom_killed();
    if (target_desc_->time_limit_ms != -1) {
        target_desc_->time_limit_exceeded = (target_desc_->time_usage_ms > target_desc_->time_limit_ms);
    }
    if (target_desc_->wall_time_limit_ms != -1) {
        target_desc_->wall_time_limit_exceeded = (target_desc_->wall_time_usage_ms > target_desc_->wall_time_limit_ms);
    }

    stop_timer();

    if (target_desc_->error) {
        die("Slave exited with error");
    }
}

void Worker::reset_wall_clock() {
    gettimeofday(&run_start_, nullptr);
}

long Worker::get_wall_clock() {
    struct timeval now = {}, seg = {};
    gettimeofday(&now, nullptr);
    timersub(&now, &run_start_, &seg);
    return seg.tv_sec * 1000 + seg.tv_usec / 1000;
}

long Worker::get_time_usage_ms() {
    std::string data = cpuacct_controller_->read("cpuacct.usage");
    return stoll(data) / 1000000;
}

long Worker::get_time_usage_sys_ms() {
    std::string data = cpuacct_controller_->read("cpuacct.usage_sys");
    return stoll(data) / 1000000;
}

long Worker::get_time_usage_user_ms() {
    std::string data = cpuacct_controller_->read("cpuacct.usage_user");
    return stoll(data) / 1000000;
}

long Worker::get_memory_usage_kb() {
    long long max_usage = stoll(memory_controller_->read("memory.max_usage_in_bytes"));
    long long cur_usage = stoll(memory_controller_->read("memory.usage_in_bytes"));
    return std::max(max_usage, cur_usage) / 1024;
}

bool Worker::is_oom_killed() {
    std::string data = memory_controller_->read("memory.oom_control");
    std::stringstream sstream(memory_controller_->read("memory.oom_control"));
    std::string name, val;
    while (sstream >> name >> val) {
        if (name == "oom_kill") {
            return (val != "0");
        }
    }
    die("Can't find oom_kill field in memory.oom_control");
}
