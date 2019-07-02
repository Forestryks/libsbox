/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include <libsbox/die.h>
#include <libsbox/execution_target.h>
#include <libsbox/execution_context.h>
#include <libsbox/init.h>
#include <libsbox/conf.h>
#include <libsbox/fs.h>
#include <libsbox/signal.h>

#include <cstring>
#include <unistd.h>
#include <signal.h>
#include <string>
#include <sys/mount.h>
#include <sys/stat.h>
#include <wait.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <grp.h>
#include <sstream>
#include <libsbox/logger.h>

int libsbox::execution_target::counter = 0;

libsbox::execution_target::execution_target(const std::vector<std::string> &argv) {
    this->global_id = counter++;
    if (argv.empty()) libsbox::die("argv length must be at least 1");
    this->argv = new char *[argv.size() + 1];
    for (int i = 0; i < (int) argv.size(); ++i) {
        this->argv[i] = strdup(argv[i].c_str());
    }
    this->argv[argv.size()] = nullptr;
    this->init();
}

libsbox::execution_target::execution_target(int argc, char **argv) {
    this->global_id = counter++;
    if (argc == 0) libsbox::die("argv length must be at least 1");
    this->argv = new char *[argc + 1];
    for (int i = 0; i < argc; ++i) {
        this->argv[i] = strdup(argv[i]);
    }
    this->argv[argc] = nullptr;
    this->init();
}

void libsbox::execution_target::init() {
    if (!initialized) libsbox::die("Cannot create execution_target while not initialized");

    this->env = new char *[1];
    this->env[0] = nullptr;

    this->stdin.name = TARGET(this) + ".stdin";
    this->stdout.name = TARGET(this) + ".stdout";
    this->stderr.name = TARGET(this) + ".stderr";

    this->bind_rules["/lib"] = {"/lib", 0};
    this->bind_rules["/lib64"] = {"/lib64", BIND_OPTIONAL};
    this->bind_rules["/usr/lib"] = {"/usr/lib", 0};
    this->bind_rules["/usr/lib64"] = {"/usr/lib64", BIND_OPTIONAL};
}

libsbox::execution_target::~execution_target() {
    for (char **ptr = this->argv; *ptr != nullptr; ++ptr) {
        // We use free() because memory was allocated in strdup() using malloc()
        free(*ptr);
    }
    delete[] (this->argv);

    for (char **ptr = this->env; *ptr != nullptr; ++ptr) {
        free(*ptr);
    }
    delete[] (this->env);
}

void libsbox::execution_target::die() {
    if (this->cpuacct_controller != nullptr) this->cpuacct_controller->die();
    if (this->memory_controller != nullptr) this->memory_controller->die();

    rmdir(this->id.c_str());

    kill(-this->proxy_pid, SIGKILL);
    kill(this->proxy_pid, SIGKILL);
}

void libsbox::execution_target::prepare() {
    this->running = true;
    this->time_usage = 0;
    this->time_usage_sys = 0;
    this->time_usage_user = 0;
    this->wall_time_usage = 0;
    this->memory_usage = 0;

    this->oom_killed = false;
    this->time_limit_exceeded = false;
    this->wall_time_limit_exceeded = false;
    this->exited = false;
    this->exit_code = -1;
    this->signaled = false;
    this->term_signal = -1;
    this->proxy_killed = false;

    this->cpuacct_controller = new cgroup_controller("cpuacct");
    this->memory_controller = new cgroup_controller("memory");

    if (this->memory_limit != -1) {
        this->memory_controller->write("memory.limit_in_bytes", std::to_string(this->memory_limit) + "K");
    }

    this->id = make_temp_dir(".", 0750);
}

void libsbox::execution_target::cleanup() {
    delete this->cpuacct_controller;
    delete this->memory_controller;

    this->cpuacct_controller = nullptr;
    this->memory_controller = nullptr;

    this->proxy_pid = 0;
    this->slave_pid = 0;

    if (rmdir(this->id.c_str()) != 0) {
        libsbox::die("Cannot remove dir %s (%s)", this->id.c_str(), strerror(errno));
    }
}

void libsbox::execution_target::prepare_root() {
    umask(0);
    if (mount("none", "/", "none", MS_REC | MS_PRIVATE, nullptr) < 0) {
        libsbox::die("Cannot privatize mounts (%s)", strerror(errno));
    }

    std::string param = "mode=755,size=" + std::to_string(root_tmpfs_size) + "k";
    if (mount("none", this->id.c_str(), "tmpfs", 0, param.c_str()) < 0) {
        libsbox::die("Cannot mount tmpfs (%s)", strerror(errno));
    }

    std::string proc_dir = join_path(this->id, "proc");
    make_path(proc_dir, 0777);
    if (mount("none", proc_dir.c_str(), "proc", 0, "") < 0) {
        libsbox::die("Cannot mount proc filesystem (%s)", strerror(errno));
    }
    if (mount("none", proc_dir.c_str(), "proc", MS_REMOUNT, "hidepid=2") < 0) {
        libsbox::die("Cannot remount proc filesystem (%s)", strerror(errno));
    }

    std::string work_dir = join_path(this->id, "work");
    make_path(work_dir, 0777);

    for (const auto &rule : this->bind_rules) {
        std::string inside, outside;
        if (rule.first[0] == '/') {
            inside = join_path(this->id, rule.first);
        } else {
            inside = join_path(work_dir, rule.first);
        }
        outside = rule.second.path;
        int flags = rule.second.flags;

        if ((flags & BIND_COPY_OUT) && !(flags & BIND_COPY_IN)) continue;

        int file_type = get_file_type(outside);
        if (!file_type) {
            if (flags & BIND_OPTIONAL) continue;
            libsbox::die("Bind %s -> %s failed: path not exists", outside.c_str(), rule.first.c_str());
        }

        if (S_ISDIR(file_type)) {
            // working with dir
            if (flags & BIND_COPY_IN) {
                libsbox::die("Bind %s -> %s failed: BIND_COPY_IN is not compatible with directories", outside.c_str(),
                             rule.first.c_str());
            }

            make_path(inside, 0777);

            unsigned long mount_flags = (MS_BIND | MS_NOSUID | MS_REC);
            if (!(flags & BIND_READWRITE)) {
                mount_flags |= MS_RDONLY;
            }

            if (mount(outside.c_str(), inside.c_str(), "none", mount_flags, "") < 0) {
                libsbox::die("Bind %s -> %s failed: mount failed (%s)", outside.c_str(), rule.first.c_str(),
                             strerror(errno));
            }
            if (mount(outside.c_str(), inside.c_str(), "none", mount_flags | MS_REMOUNT, "") < 0) {
                libsbox::die("Bind %s -> %s failed: remount failed (%s)", outside.c_str(), rule.first.c_str(),
                             strerror(errno));
            }
        } else {
            if (flags & BIND_COPY_IN) {
                if (!S_ISREG(file_type)) {
                    libsbox::die("Bind %s -> %s failed: BIND_COPY_IN is compatible only with regular files",
                                 outside.c_str(), rule.first.c_str());
                }
                make_file(inside, 0777, 0666);
                if (flags & BIND_READWRITE) copy_file(outside, inside, 0666);
                else copy_file(outside, inside, 0644);
                continue;
            }

            make_file(inside, 0777, 0666);

            unsigned long mount_flags = (MS_BIND | MS_NOSUID);
            if (!(flags & BIND_READWRITE)) {
                mount_flags |= MS_RDONLY;
            }

            if (mount(outside.c_str(), inside.c_str(), "none", mount_flags, "") < 0) {
                libsbox::die("Bind %s -> %s failed: mount failed (%s)", outside.c_str(), rule.first.c_str(),
                             strerror(errno));
            }
            if (mount(outside.c_str(), inside.c_str(), "none", mount_flags | MS_REMOUNT, "") < 0) {
                libsbox::die("Bind %s -> %s failed: remount failed (%s)", outside.c_str(), rule.first.c_str(),
                             strerror(errno));
            }
        }
    }
    umask(022);
}

void libsbox::execution_target::destroy_root() {
    std::string work_dir = join_path(this->id, "work");
    for (auto &rule : this->bind_rules) {
        std::string inside, outside;
        if (rule.first[0] == '/') {
            inside = join_path(this->id, rule.first);
        } else {
            inside = join_path(work_dir, rule.first);
        }
        outside = rule.second.path;
        int flags = rule.second.flags;

        if (!(flags & BIND_COPY_OUT)) continue;

        int file_type = get_file_type(inside);
        if (!file_type) {
            if (flags & BIND_OPTIONAL) continue;
            libsbox::die("Copying out %s <- %s failed: path not exists", outside.c_str(), rule.first.c_str());
        }

        if (!S_ISREG(file_type)) {
            if (flags & BIND_OPTIONAL) continue;
            libsbox::die("Copying out %s <- %s failed: BIND_COPY_OUT is compatible only with regular files",
                         outside.c_str(), rule.first.c_str());
        }

        make_file(outside, 0755, 0644);
        copy_file(inside, outside, 0644);
    }
}

int libsbox::execution_target::proxy() {
    reset_signals();

    if (close(this->status_pipe[0]) != 0) {
        libsbox::die("Cannot close read end of status pipe (%s)", strerror(errno));
    }
    if (close(current_context->error_pipe[0]) != 0) {
        libsbox::die("Cannot close read end of error pipe (%s)", strerror(errno));
    }

    this->prepare_root();

    this->slave_pid = fork();
    if (this->slave_pid < 0) {
        libsbox::die("Cannot start slave process: fork failed (%s)", strerror(errno));
    }
    if (this->slave_pid == 0) {
        this->slave();
    }

    int stat;
    pid_t pid = waitpid(this->slave_pid, &stat, 0);
    if (pid != this->slave_pid) {
        libsbox::die("Wait for slave process failed (%s)", strerror(errno));
    }

    if (write(this->status_pipe[1], &stat, sizeof(stat)) != sizeof(stat)) {
        libsbox::die("Cannot write exit status to status pipe");
    }

    this->destroy_root();

    return 0;
}

namespace libsbox {
    int clone_callback(void *);
} // namespace libsbox

int libsbox::clone_callback(void *target) {
    current_target = (execution_target *) target;
    return current_target->proxy();
}

void libsbox::execution_target::start_proxy() {
    char *clone_stack = new char[clone_stack_size];
    // CLONE_NEWIPC and CLONE_NEWNS are dramatically slow
    // I don't know why
    this->proxy_pid = clone(
            clone_callback,
            clone_stack + clone_stack_size,
            SIGCHLD | CLONE_NEWNS | CLONE_NEWIPC | CLONE_NEWNET | CLONE_NEWPID,
            this
    );
    delete[] clone_stack;

    if (this->proxy_pid < 0) {
        libsbox::die("Cannot create proxy: fork failed (%s)", strerror(errno));
    }

    if (close(this->status_pipe[1]) != 0) {
        libsbox::die("Cannot close write end of status pipe (%s)", strerror(errno));
    }
}

void libsbox::execution_target::freopen_fds() {
    // TODO: rules
    if (!this->stdin.filename.empty()) {
        this->stdin.fd = open(this->stdin.filename.c_str(), O_RDONLY);
        if (this->stdin.fd < 0) {
            libsbox::die("Cannot open %s (%s)", this->stdin.filename.c_str(), strerror(errno));
        }
    }

    if (!this->stdout.filename.empty()) {
        this->stdout.fd = open(this->stdout.filename.c_str(), O_WRONLY | O_CREAT);
        if (this->stdout.fd < 0) {
            libsbox::die("Cannot open %s (%s)", this->stdout.filename.c_str(), strerror(errno));
        }
    }

    if (!this->stderr.filename.empty()) {
        this->stderr.fd = open(this->stderr.filename.c_str(), O_WRONLY | O_CREAT);
        if (this->stderr.fd < 0) {
            libsbox::die("Cannot open %s (%s)", this->stderr.filename.c_str(), strerror(errno));
        }
    }
}

void libsbox::execution_target::dup2_fds() {
    if (close(0) != 0) {
        libsbox::die("Cannot close stdin");
    }

    if (close(1) != 0) {
        libsbox::die("Cannot close stdout");
    }

    if (close(2) != 0) {
        libsbox::die("Cannot close stderr");
    }

    if (this->stdin.fd != -1) {
        if (dup2(this->stdin.fd, 0) != 0) {
            libsbox::die("Cannot dup2 stdin (%s)", strerror(errno));
        }
    }

    if (this->stdout.fd != -1) {
        if (dup2(this->stdout.fd, 1) != 1) {
            libsbox::die("Cannot dup2 stdout (%s)", strerror(errno));
        }
    }

    if (this->stderr.fd != -1) {
        if (dup2(this->stderr.fd, 2) != 2) {
            libsbox::die("Cannot dup2 stderr (%s)", strerror(errno));
        }
    }
}

void libsbox::execution_target::close_all_fds() {
    DIR *dir = opendir("/proc/self/fd");
    if (dir == nullptr) {
        libsbox::die("Cannot open /proc/self/fd (%s)", strerror(errno));
    }
    int dir_fd = dirfd(dir);
    if (dir_fd < 0) {
        libsbox::die("Cannot get fd from DIR* (%s)", strerror(errno));
    }

    struct dirent *dentry;
    while ((dentry = readdir(dir))) {
        char *end;
        long fd = strtol(dentry->d_name, &end, 10);
        if (*end) continue;

        if ((fd >= 0 && fd <= 2) || fd == dir_fd || fd == current_context->error_pipe[1] ||
            fd == this->exec_fd)
            continue;
        if (close(fd) != 0) {
            libsbox::die("Cannot close fd %ld (%s)", fd, strerror(errno));
        }
    }

    if (closedir(dir) != 0) {
        libsbox::die("Cannot close /proc/");
    }
}

void set_rlimit_ext(const char *res_name, int res, rlim_t limit) {
    struct rlimit rlim = {limit, limit};
    if (setrlimit(res, &rlim) != 0) {
        libsbox::die("Cannot set %s (%s)", res_name, strerror(errno));
    }
}

#define set_rlimit(res, limit) set_rlimit_ext(#res, res, limit)

void libsbox::execution_target::setup_rlimits() {
    if (this->fsize_limit != -1) {
        set_rlimit(RLIMIT_FSIZE, this->fsize_limit);
    }

    set_rlimit(RLIMIT_STACK, RLIM_INFINITY);
    set_rlimit(RLIMIT_NOFILE, (this->max_files == -1 ? RLIM_INFINITY : this->max_files));
    set_rlimit(RLIMIT_NPROC, (this->max_threads == -1 ? RLIM_INFINITY : this->max_threads));
    set_rlimit(RLIMIT_MEMLOCK, 0);
}

#undef set_rlimit

void libsbox::execution_target::setup_credentials() {
    if (setresgid(this->uid, this->uid, this->uid) != 0) {
        libsbox::die("Setting process gid to %d failed (%s)", this->uid, strerror(errno));
    }
    if (setgroups(0, nullptr) != 0) {
        libsbox::die("Removing process from all groups failed (%s)", strerror(errno));
    }
    if (setresuid(this->uid, this->uid, this->uid) != 0) {
        libsbox::die("Setting process uid to %d failed (%s)", this->uid, strerror(errno));
    }
    if (setpgrp() != 0) {
        libsbox::die("Setting process group id failed (%s)", strerror(errno));
    }
}

[[noreturn]]
void libsbox::execution_target::slave() {
    this->inside_box = true;
    this->exec_fd = open(this->argv[0], O_RDONLY | O_CLOEXEC);
    if (exec_fd < 0) {
        libsbox::die("Cannot open target executable");
    }
    fchmod(this->exec_fd, 777);

    std::string new_cwd = join_path(this->id, "work");
    if (chdir(new_cwd.c_str()) != 0) {
        libsbox::die("Chdir to %s failed (%s)", new_cwd.c_str(), strerror(errno));
    }

    this->freopen_fds();
    this->dup2_fds();
    this->close_all_fds();
    this->setup_rlimits();
    this->memory_controller->enter();
    this->cpuacct_controller->enter();

    if (chroot("..") != 0) {
        libsbox::die("Chroot failed (%s)", strerror(errno));
    }

    this->setup_credentials();

    fexecve(this->exec_fd, this->argv, this->env);
    libsbox::die("Failed to exec target (%s)", strerror(errno));
}

long libsbox::execution_target::get_time_usage() {
    std::string data = this->cpuacct_controller->read("cpuacct.usage");
    return stoll(data) / 1000000;
}

long libsbox::execution_target::get_time_usage_sys() {
    std::string data = this->cpuacct_controller->read("cpuacct.usage_sys");
    return stoll(data) / 1000000;
}

long libsbox::execution_target::get_time_usage_user() {
    std::string data = this->cpuacct_controller->read("cpuacct.usage_user");
    return stoll(data) / 1000000;
}

long libsbox::execution_target::get_memory_usage() {
    long long max_usage = stoll(this->memory_controller->read("memory.max_usage_in_bytes"));
    long long cur_usage = stoll(this->memory_controller->read("memory.usage_in_bytes"));
    return std::max(max_usage, cur_usage) / 1024;
}

bool libsbox::execution_target::get_oom_status() {
    std::string data = this->memory_controller->read("memory.oom_control");
    std::stringstream sstream(this->memory_controller->read("memory.oom_control"));
    std::string name, val;
    while (sstream >> name >> val) {
        if (name == "oom_kill") {
            return (val != "0");
        }
    }
    libsbox::die("Can't find oom_kill field in memory.oom_control");
}

std::string libsbox::execution_target::json_params() {
    std::string res = "{";

    res += "'argv': [";
    for (int i = 0; this->argv[i] != nullptr; ++i) {
        if (i != 0) {
            res += ", ";
        }
        res += "'";
        res += this->argv[i];
        res += "'";
    }
    res += "], ";

    res += "'env': [";
    for (int i = 0; this->env[i] != nullptr; ++i) {
        if (i != 0) {
            res += ", ";
        }
        res += "'";
        res += this->env[i];
        res += "'";
    }
    res += "], ";

    res += "'time_limit': " + std::to_string(this->time_limit) + ", ";
    res += "'memory_limit': " + std::to_string(this->memory_limit) + ", ";
    res += "'fsize_limit': " + std::to_string(this->fsize_limit) + ", ";
    res += "'max_files': " + std::to_string(this->max_files) + ", ";
    res += "'max_threads': " + std::to_string(this->max_threads) + ", ";
    res += "'uid': " + std::to_string(this->uid) + ", ";

    if (!this->stdin.filename.empty()) {
        res += "'stdin': '" + this->stdin.filename + "', ";
    }
    if (!this->stdout.filename.empty()) {
        res += "'stdout': '" + this->stdout.filename + "', ";
    }
    if (!this->stderr.filename.empty()) {
        res += "'stderr': '" + this->stderr.filename + "', ";
    }

    res += "'bind_rules': [";
    bool first = true;
    for (const auto &it : this->bind_rules) {
        if (!first) {
            res += ", ";
        }
        res += "{";
        res += "'inside': '" + it.first + "', ";
        res += "'outside': '" + it.second.path + "', ";
        res += "'flags': " + std::to_string(it.second.flags);
        res += "}";
        first = false;
    }
    res += "]";
    res += "}";
    return res;
}

namespace std {
    string to_string(bool t) {
        if (t) return "true";
        return "false";
    }
}

std::string libsbox::execution_target::json_results(bool readable) {
    std::string res = "{";
    if (readable) {
        res += "'time_usage': '" + std::to_string(this->time_usage) + "ms', ";
        res += "'time_usage_sys': '" + std::to_string(this->time_usage_sys) + "ms', ";
        res += "'time_usage_user': '" + std::to_string(this->time_usage_user) + "ms', ";
        res += "'wall_time_usage': '" + std::to_string(this->wall_time_usage) + "ms', ";
        res += "'memory_usage': '" + std::to_string(this->memory_usage) + "kb', ";
        res += "'time_limit_exceeded': " + std::to_string(this->time_limit_exceeded) + ", ";
        res += "'wall_time_limit_exceeded': " + std::to_string(this->wall_time_limit_exceeded) + ", ";
        res += "'exited': " + std::to_string(this->exited) + ", ";
        res += "'exit_code': " + std::to_string(this->exit_code) + ", ";
        res += "'signaled': " + std::to_string(this->signaled) + ", ";
        res += "'term_signal': " + std::to_string(this->term_signal) + ", ";
        res += "'oom_killed': " + std::to_string(this->oom_killed);
    } else {
        res += "'time_usage': " + std::to_string(this->time_usage) + ", ";
        res += "'time_usage_sys': " + std::to_string(this->time_usage_sys) + ", ";
        res += "'time_usage_user': " + std::to_string(this->time_usage_user) + ", ";
        res += "'wall_time_usage': " + std::to_string(this->wall_time_usage) + ", ";
        res += "'memory_usage': " + std::to_string(this->memory_usage) + ", ";
        res += "'time_limit_exceeded': " + std::to_string((int)this->time_limit_exceeded) + ", ";
        res += "'wall_time_limit_exceeded': " + std::to_string((int)this->wall_time_limit_exceeded) + ", ";
        res += "'exited': " + std::to_string((int)this->exited) + ", ";
        res += "'exit_code': " + std::to_string(this->exit_code) + ", ";
        res += "'signaled': " + std::to_string((int)this->signaled) + ", ";
        res += "'term_signal': " + std::to_string(this->term_signal) + ", ";
        res += "'oom_killed': " + std::to_string((int)this->oom_killed);
    }
    res += "}";

    return res;
}

std::string libsbox::execution_target::plain_results() {
    std::string res;
    res += "time_usage " + std::to_string(this->time_usage) + "\n";
    res += "time_usage_sys " + std::to_string(this->time_usage_sys) + "\n";
    res += "time_usage_user " + std::to_string(this->time_usage_user) + "\n";
    res += "wall_time_usage " + std::to_string(this->wall_time_usage) + "\n";
    res += "memory_usage " + std::to_string(this->memory_usage) + "\n";
    res += "time_limit_exceeded " + std::to_string((int)this->time_limit_exceeded) + "\n";
    res += "wall_time_limit_exceeded " + std::to_string((int)this->wall_time_limit_exceeded) + "\n";
    res += "exited " + std::to_string((int)this->exited) + "\n";
    res += "exit_code " + std::to_string(this->exit_code) + "\n";
    res += "signaled " + std::to_string((int)this->signaled) + "\n";
    res += "term_signal " + std::to_string(this->term_signal) + "\n";
    res += "oom_killed " + std::to_string((int)this->oom_killed);
    return res;
}

// Order is like in plain_results
std::string libsbox::execution_target::min_results() {
    std::string res;
    res += std::to_string(this->time_usage) + " ";
    res += std::to_string(this->time_usage_sys) + " ";
    res += std::to_string(this->time_usage_user) + " ";
    res += std::to_string(this->wall_time_usage) + " ";
    res += std::to_string(this->memory_usage) + " ";
    res += std::to_string((int)this->time_limit_exceeded) + " ";
    res += std::to_string((int)this->wall_time_limit_exceeded) + " ";
    res += std::to_string((int)this->exited) + " ";
    res += std::to_string(this->exit_code) + " ";
    res += std::to_string((int)this->signaled) + " ";
    res += std::to_string(this->term_signal) + " ";
    res += std::to_string((int)this->oom_killed);
    return res;
}


namespace libsbox {
    execution_target *current_target = nullptr;
} // namespace libsbox
