/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_WORKER_H_
#define LIBSBOX_WORKER_H_

#include <libsbox/shared_memory.h>
#include <libsbox/shared_barrier.h>
#include <libsbox/target_desc.h>
#include <libsbox/cgroup.h>

#include <filesystem>
#include <sys/resource.h>

namespace fs = std::filesystem;

class Worker : Context {
public:
    explicit Worker(int uid);
    ~Worker();

    static Worker &get();

    void start();

    [[nodiscard]] SharedBarrier *get_desc_write_barrier();
    [[nodiscard]] target_desc *get_target_desc() const;
    [[noreturn]] void die(const std::string &error) override;
private:
    static Worker *worker_;

    SharedBarrier desc_write_barrier_;
    SharedMemory *shared_memory_;
    target_desc *target_desc_;

    uid_t uid_;
    fs::path root_;
    fs::path work_dir_;
    pid_t slave_pid_ = -1;

    int serve();
    void prepare();
    void prepare_root();
    void prepare_ipcs();

    void kill_all();

    struct timeval run_start_;

    void wait_for_target();
    void reset_wall_clock();
    long get_wall_clock();
    long get_time_usage_ms();
    long get_time_usage_sys_ms();
    long get_time_usage_user_ms();
    long get_memory_usage_kb();
    bool is_oom_killed();

    int exec_fd_;
    CgroupController *cpuacct_controller_ = nullptr;
    CgroupController *memory_controller_ = nullptr;

    [[noreturn]]
    void slave();
    void freopen_fds();
    void dup2_fds();
    void close_all_fds();
    void setup_rlimits();
    void set_rlimit_ext(const char *res_name, int res, rlim_t limit);
    void setup_credentials();

    friend int clone_callback(void *ptr);
};

#endif //LIBSBOX_WORKER_H_
