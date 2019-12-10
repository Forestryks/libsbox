/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include "container.h"
#include "worker.h"
#include "bind.h"
#include "config.h"
#include "utils.h"
#include "signals.h"
#include "logger.h"

#include <unistd.h>
#include <signal.h>
#include <sys/prctl.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <fcntl.h>
#include <grp.h>
#include <dirent.h>
#include <sys/stat.h>

Container *Container::container_ = nullptr;

Container::Container(uid_t id, bool permanent) : id_(id), permanent_(permanent) {}

void Container::_die(const std::string &error) {
    if (slave_pid_ == 0) {
        task_data_->error = true;
    } else {
        if (cpuacct_controller_ != nullptr) cpuacct_controller_->_die();
        if (memory_controller_ != nullptr) memory_controller_->_die();
    }
    log(error);

    _exit(1);
}

void Container::terminate() {
    die("Container should never be terminated");
}

uid_t Container::get_id() {
    return id_;
}

pid_t Container::get_pid() {
    return pid_;
}

SharedBarrier *Container::get_barrier() {
    return &barrier_;
}

void Container::set_task(libsbox::Task *task) {
    task_data_->time_limit_ms = task->get_time_limit_ms();
    task_data_->wall_time_limit_ms = task->get_wall_time_limit_ms();
    task_data_->memory_limit_kb = task->get_memory_limit_kb();
    task_data_->fsize_limit_kb = task->get_fsize_limit_kb();
    task_data_->max_files = task->get_max_files();
    task_data_->max_threads = task->get_max_threads();
    task_data_->need_ipc = task->get_need_ipc();
    task_data_->use_standard_binds = task->get_use_standard_binds();

    task_data_->stdin_desc.fd = -1;
    task_data_->stdin_desc.filename = "";
    std::string stdin_filename = task->get_stdin().get_filename();
    if (stdin_filename.empty()) {
        task_data_->stdin_desc.filename = "/dev/null";
    } else {
        if (stdin_filename[0] == '@') {
            const auto &pipe = Worker::get().get_pipe(stdin_filename.substr(1));
            task_data_->stdin_desc.fd = pipe.first;
        } else {
            if (stdin_filename.size() > task_data_->stdin_desc.filename.max_size()) {
                die(format(
                    "stdin filename is larger than maximum (%zi > %zi)",
                    stdin_filename.size(),
                    task_data_->stdin_desc.filename.max_size()));
            }
            task_data_->stdin_desc.filename = stdin_filename;
        }
    }

    task_data_->stdout_desc.fd = -1;
    task_data_->stdout_desc.filename = "";
    std::string stdout_filename = task->get_stdout().get_filename();
    if (stdout_filename.empty()) {
        task_data_->stdout_desc.filename = "/dev/null";
    } else {
        if (stdout_filename[0] == '@') {
            const auto &pipe = Worker::get().get_pipe(stdout_filename.substr(1));
            task_data_->stdout_desc.fd = pipe.second;
        } else {
            if (stdout_filename.size() > task_data_->stdout_desc.filename.max_size()) {
                die(format(
                    "stdout filename is larger than maximum (%zi > %zi)",
                    stdout_filename.size(),
                    task_data_->stdout_desc.filename.max_size()));
            }
            task_data_->stdout_desc.filename = stdout_filename;
        }
    }

    task_data_->stderr_desc.fd = -1;
    task_data_->stderr_desc.filename = "";
    std::string stderr_filename = task->get_stderr().get_filename();
    if (stderr_filename.empty()) {
        task_data_->stderr_desc.filename = "/dev/null";
    } else {
        if (stderr_filename[0] == '@') {
            std::string pipe_name = stderr_filename.substr(1);
            if (pipe_name == "_stdout") {
                task_data_->stderr_desc.fd = STDOUT_FILENO;
            } else {
                const auto &pipe = Worker::get().get_pipe(pipe_name);
                task_data_->stderr_desc.fd = pipe.second;
            }
        } else {
            if (stderr_filename.size() > task_data_->stderr_desc.filename.max_size()) {
                die(format(
                    "stderr filename is larger than maximum (%zi > %zi)",
                    stderr_filename.size(),
                    task_data_->stderr_desc.filename.max_size()));
            }
            task_data_->stderr_desc.filename = stderr_filename;
        }
    }

    size_t argv_size = task->get_argv().size();
    if (argv_size > task_data_->argv.max_count()) {
        die(format("argv size is larger than maximum (%zi > %zi)", argv_size, task_data_->argv.max_count()));
    }
    size_t total_size = 0;
    for (size_t i = 0; i < argv_size; ++i) {
        total_size += task->get_argv()[i].size();
    }
    if (total_size > task_data_->argv.max_size()) {
        die(format("argv total size is larger than maximum (%zi > %zi)", total_size, task_data_->argv.max_size()));
    }
    task_data_->argv.clear();
    for (const auto &arg : task->get_argv()) {
        task_data_->argv.add(arg);
    }

    size_t env_size = task->get_env().size();
    if (env_size > task_data_->env.max_count()) {
        die(format("env size is larger than maximum (%zi > %zi)", env_size, task_data_->env.max_count()));
    }
    total_size = 0;
    for (size_t i = 0; i < env_size; ++i) {
        total_size += task->get_env()[i].size();
    }
    if (total_size > task_data_->env.max_size()) {
        die(format("env total size is larger than maximum (%zi > %zi)", total_size, task_data_->env.max_size()));
    }
    task_data_->env.clear();
    for (const auto &arg : task->get_env()) {
        task_data_->env.add(arg);
    }

    size_t binds_count = task->get_binds().size();
    if (binds_count > task_data_->binds.max_size()) {
        die(format("binds count is larger that maximum (%zi > %zi)", binds_count, task_data_->binds.max_size()));
    }
    task_data_->binds.clear();
    for (const auto &bind : task->get_binds()) {
        std::string inside = bind.get_inside_path();
        std::string outside = bind.get_outside_path();
        int flags = bind.get_flags();
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
    task_data_->memory_limit_hit = false;

    task_data_->error = true;
}

void Container::put_results(libsbox::Task *task) {
    task->set_time_usage_ms(task_data_->time_usage_ms);
    task->set_time_usage_sys_ms(task_data_->time_usage_sys_ms);
    task->set_time_usage_user_ms(task_data_->time_usage_user_ms);
    task->set_wall_time_usage_ms(task_data_->wall_time_usage_ms);
    task->set_memory_usage_kb(task_data_->memory_usage_kb);
    task->set_time_limit_exceeded(task_data_->time_limit_exceeded);
    task->set_wall_time_limit_exceeded(task_data_->wall_time_limit_exceeded);
    task->set_exited(task_data_->exited);
    task->set_exit_code(task_data_->exit_code);
    task->set_signaled(task_data_->signaled);
    task->set_term_signal(task_data_->term_signal);
    task->set_oom_killed(task_data_->oom_killed);
    task->set_memory_limit_hit(task_data_->memory_limit_hit);
}

int Container::clone_callback(void *ptr) {
    static_cast<Container *>(ptr)->serve();
    return 0;
}

pid_t Container::start() {
    const size_t clone_stack_size = 8 * 1024 * 1024;
    char *clone_stack = new char[clone_stack_size];
    // SIGCHLD - send SIGCHLD on exit
    // CLONE_FILES - share file descriptors table
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
    ContextManager::set(this, "container");
    container_ = this;
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
        memory_controller_->write("memory.swappiness", "0");
        if (task_data_->memory_limit_kb != -1) {
            memory_controller_->write("memory.limit_in_bytes", std::to_string(task_data_->memory_limit_kb) + "K");
        }

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
            bind.umount_if_mounted();
        }

        delete cpuacct_controller_;
        cpuacct_controller_ = nullptr;
        delete memory_controller_;
        memory_controller_ = nullptr;

        // Results ready
        barrier_.wait();

        if (!permanent_) break;

        cleanup_root();
    }

    _exit(0);
}

void Container::prepare() {
    root_ = Config::get().get_box_dir();

    if (prctl(PR_SET_PDEATHSIG, SIGKILL) != 0) {
        die(format("Cannot set parent death signal: %m"));
    }

    reset_signals();
    enable_timer_interrupts();
    set_sigchld_action(sigchld_action_wrapper);

    prepare_root();
    if (permanent_) {
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
        die(format("Cannot create root directory (%s): %s", root_.c_str(), error.message().c_str()));
    }

    if (mount("none", root_.c_str(), "tmpfs", 0, "mode=755,size=1g") != 0) {
        die(format("Cannot mount root tmpfs: %m"));
    }
    if (mount("none", root_.c_str(), "tmpfs", MS_REMOUNT, "mode=755,size=1g") != 0) {
        die(format("Cannot remount root tmpfs: %m"));
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
        die(format("Cannot create '/work' dir: %s", error.message().c_str()));
    }

    fs::create_directories(root_ / "tmp");
    if (error) {
        die(format("Cannot create '/tmp' dir: %s", error.message().c_str()));
    }
    if (chmod((root_ / "tmp").c_str(), 0777) != 0) {
        die(format("Cannot chmod() '/tmp': %m"));
    }

    if (task_data_->use_standard_binds) {
        Bind::apply_standard_rules(root_, work_dir_);
    }
}

void Container::cleanup_root() {
    std::error_code error;
    for (const auto &path : fs::directory_iterator(work_dir_)) {
        fs::remove_all(path, error);
        if (error) {
            die(format("Cannot remove %s: %s", path.path().c_str(), error.message().c_str()));
        }
    }

    for (const auto &path : fs::directory_iterator(root_ / "tmp")) {
        fs::remove_all(path, error);
        if (error) {
            die(format("Cannot remove %s: %s", path.path().c_str(), error.message().c_str()));
        }
    }

    for (const auto &path : fs::directory_iterator(work_dir_)) {
        fs::remove_all(path, error);
        if (error) {
            die(format("Cannot remove %s: %s", path.path().c_str(), error.message().c_str()));
        }
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

            if (task_data_->wall_time_limit_ms != -1 && get_wall_clock_ms() > task_data_->wall_time_limit_ms) {
                kill_all();
                break;
            }

            continue;
        }

        kill_all();

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
    task_data_->wall_time_usage_ms = get_wall_clock_ms();
    task_data_->memory_usage_kb = get_memory_usage_kb();
    task_data_->oom_killed = is_oom_killed();
    task_data_->memory_limit_hit = is_memory_limit_hit();
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
    stop_timer();
    if (kill(-1, SIGKILL) != 0 && errno != ESRCH) {
        die(format("Failed to kill all processes in box: %m"));
    }
    set_standard_handler_restart(SIGALRM, true);
    while (true) {
        int status;
        pid_t pid = wait(&status);
        if (pid < 0) {
            if (errno == ECHILD) {
                break;
            }
            die(format("kill_all() wait() failed: %m"));
        }
    }
    set_standard_handler_restart(SIGALRM, false);
}

void Container::reset_wall_clock() {
    gettimeofday(&run_start_, nullptr);
}

time_ms_t Container::get_wall_clock_ms() {
    struct timeval now = {}, seg = {};
    gettimeofday(&now, nullptr);
    timersub(&now, &run_start_, &seg);
    return seg.tv_sec * 1000 + seg.tv_usec / 1000;
}

time_ms_t Container::get_time_usage_ms() {
    std::string data = cpuacct_controller_->read("cpuacct.usage");
    return stoll(data) / 1000000;
}

time_ms_t Container::get_time_usage_sys_ms() {
    std::string data = cpuacct_controller_->read("cpuacct.usage_sys");
    return stoll(data) / 1000000;
}

time_ms_t Container::get_time_usage_user_ms() {
    std::string data = cpuacct_controller_->read("cpuacct.usage_user");
    return stoll(data) / 1000000;
}

memory_kb_t Container::get_memory_usage_kb() {
    long long max_usage = stoll(memory_controller_->read("memory.max_usage_in_bytes"));
    long long cur_usage = stoll(memory_controller_->read("memory.usage_in_bytes"));
    return static_cast<memory_kb_t>(std::max(max_usage, cur_usage) / 1024);
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

bool Container::is_memory_limit_hit() {
    std::string data = memory_controller_->read("memory.failcnt");
    return stoll(data);
}

void Container::open_files() {
    if (!task_data_->stdin_desc.filename.empty()) {
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
    if (task_data_->stdin_desc.fd != -1) {
        if (dup2(task_data_->stdin_desc.fd, STDIN_FILENO) != STDIN_FILENO) {
            die(format("Cannot dup2 stdin: %m"));
        }
    }

    if (task_data_->stdout_desc.fd != -1) {
        if (dup2(task_data_->stdout_desc.fd, STDOUT_FILENO) != STDOUT_FILENO) {
            die(format("Cannot dup2 stdout: %m"));
        }
    }

    if (task_data_->stderr_desc.fd != -1) {
        if (dup2(task_data_->stderr_desc.fd, STDERR_FILENO) != STDERR_FILENO) {
            die(format("Cannot dup2 stderr: %m"));
        }
    }
}

void Container::close_all_fds() {
    DIR *dir = opendir("/proc/self/fd");
    if (dir == nullptr) {
        die(format("Cannot open /proc/self/fd: %m"));
    }
    fd_t dir_fd = dirfd(dir);
    if (dir_fd < 0) {
        die(format("Cannot get fd from DIR*: %m"));
    }

    struct dirent *dentry;
    while ((dentry = readdir(dir))) {
        char *end;
        fd_t fd = strtol(dentry->d_name, &end, 10);
        if (*end) continue;

        if (fd == STDIN_FILENO || fd == STDOUT_FILENO || fd == STDERR_FILENO || fd == dir_fd
            || fd == Logger::get().get_fd() || fd == memory_controller_->get_enter_fd()
            || fd == cpuacct_controller_->get_enter_fd())
            continue;
        if (close(fd) != 0) {
            die(format("Cannot close fd %d: %m", fd));
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
        set_rlimit(RLIMIT_FSIZE, static_cast<unsigned>(task_data_->fsize_limit_kb));
    }

    set_rlimit(RLIMIT_CORE, 0);
    set_rlimit(RLIMIT_STACK, RLIM_INFINITY);
    if (task_data_->max_files != -1) {
        set_rlimit(RLIMIT_NOFILE, static_cast<unsigned>(task_data_->max_files));
    }
    set_rlimit(RLIMIT_NPROC,
        (task_data_->max_threads == -1 ? RLIM_INFINITY : static_cast<unsigned>(task_data_->max_threads)));
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

#include <iostream>

void Container::slave() {
    ContextManager::set(this, "slave");
    reset_sigchld();

    Worker::get().get_run_start_barrier()->wait();
    task_data_->error = false;

    memory_controller_->delay_enter();
    cpuacct_controller_->delay_enter();

    if (chdir(work_dir_.c_str()) != 0) {
        die(format("chdir() failed: %m"));
    }

    if (chroot(root_.c_str()) != 0) {
        die(format("chroot() failed: %m"));
    }

    open_files();
    dup2_fds();
    close_all_fds();
    setup_rlimits();
    setup_credentials();

    memory_controller_->enter();
    cpuacct_controller_->enter();

    bool has_path = false;
    for (size_t i = 0; i < task_data_->env.count(); ++i) {
        if (strcmp(task_data_->env[i], "PATH=") == 0) {
            has_path = true;
            break;
        }
    }
    if (!has_path) {
        char *path_env = getenv("PATH");
        if (path_env != nullptr) {
            task_data_->env.add(format("PATH=%s", path_env));
        }
    }

    execvpe(task_data_->argv[0], task_data_->argv.get(), task_data_->env.get());
    die(format("Failed to execute command '%s': %m", task_data_->argv[0]));
    _exit(-1); // we should not get here
}

void Container::sigchld_action_wrapper(int, siginfo_t *siginfo, void *) {
    container_->sigchld_action(siginfo);
}

Container &Container::get() {
    return *container_;
}

void Container::sigchld_action(siginfo_t *siginfo) {
    if (!task_data_->error) return;
    if (siginfo->si_code != CLD_EXITED || siginfo->si_status != 0) {
        if (siginfo->si_code == CLD_EXITED) {
            die(format("Slave exited with exit code %d", siginfo->si_status));
        } else {
            die(format("Slave received signal %d (%s)", siginfo->si_status, strsignal(siginfo->si_status)));
        }
    }
}
