/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_CONTAINER_H
#define LIBSBOX_CONTAINER_H

#include "context_manager.h"
#include "shared_barrier.h"
#include "shared_memory_object.h"
#include "task_data.h"
#include "cgroup_controller.h"
#include "libsbox_internal.h"

#include <filesystem>
#include <sys/resource.h>
#include <signal.h>

namespace fs = std::filesystem;

class Container final : public ContextManager {
public:
    Container(uid_t id, bool permanent);
    ~Container() = default;

    static Container &get();

    pid_t start();
    void set_task(libsbox::Task *task);
    void put_results(libsbox::Task *task);

    uid_t get_id();
    pid_t get_pid();
    SharedBarrier *get_barrier();

    [[noreturn]]
    void _die(const std::string &error) override;
    void terminate() override;
private:
    static Container *container_;

    uid_t id_;
    pid_t pid_{};
    bool permanent_;
    SharedMemoryObject<TaskData> task_data_{};
    SharedBarrier barrier_{2};
    fs::path root_;
    fs::path work_dir_;

    CgroupController *cpuacct_controller_ = nullptr;
    CgroupController *memory_controller_ = nullptr;
    pid_t slave_pid_ = -1;
    struct timeval run_start_ = {};

    static int clone_callback(void *ptr);
    void serve();
    void prepare();
    void prepare_root();
    void disable_ipcs();
    void cleanup_root();
    void wait_for_slave();
    void kill_all();
    void reset_wall_clock();
    time_ms_t get_wall_clock_ms();
    time_ms_t get_time_usage_ms();
    time_ms_t get_time_usage_sys_ms();
    time_ms_t get_time_usage_user_ms();
    memory_kb_t get_memory_usage_kb();
    bool is_oom_killed();
    bool is_memory_limit_hit();

    [[noreturn]]
    void slave();
    void open_files();
    void dup2_fds();
    void close_all_fds();
    void setup_rlimits();
    void set_rlimit_ext(const char *res_name, int res, rlim_t limit);
    void setup_credentials();

    static void sigchld_action_wrapper(int, siginfo_t *siginfo, void *);
    void sigchld_action(siginfo_t *siginfo);
};

#endif //LIBSBOX_CONTAINER_H
