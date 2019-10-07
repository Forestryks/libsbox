/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_CONTAINER_H_
#define LIBSBOX_CONTAINER_H_

#include <libsbox/context_manager.h>
#include <libsbox/limits.h>
#include <libsbox/task_data.h>
#include <libsbox/shared_memory.h>
#include <libsbox/shared_waiter.h>
#include <libsbox/cgroup_controller.h>

#include <filesystem>
#include <json.hpp>
#include <zconf.h>
#include <sys/resource.h>

namespace fs = std::filesystem;

class Container : public ContextManager {
public:
    explicit Container(int id);
    ~Container() = default;

    static Container &get();

    pid_t start();
    void get_task_data_from_json(const nlohmann::json &json_task);
    nlohmann::json results_to_json();

    [[noreturn]]
    void die(const std::string &error) override;
    void terminate() override;
private:
    static Container *container_;

    int id_;
    SharedMemory<TaskData> task_data_;
    SharedWaiter task_data_write_waiter_;
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
    void prepare_ipcs();
    void wait_for_slave();
    void kill_all();
    void reset_wall_clock();
    long get_wall_clock();
    long get_time_usage_ms();
    long get_time_usage_sys_ms();
    long get_time_usage_user_ms();
    long get_memory_usage_kb();
    bool is_oom_killed();

    int exec_fd_ = -1;

    [[noreturn]]
    void slave();
    void freopen_fds();
    void dup2_fds();
    void close_all_fds();
    void setup_rlimits();
    void set_rlimit_ext(const char *res_name, int res, rlim_t limit);
    void setup_credentials();
};

#endif //LIBSBOX_CONTAINER_H_
