/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include <libsbox.h>
#include <memory>
#include <iostream>

void fatal_handler(const char *msg) {
    std::cout << "Error: " << msg << std::endl;
}

int main() {
    libsbox::init(fatal_handler);

    std::unique_ptr<libsbox::execution_target> solution_target(new libsbox::execution_target({"solution"}));

    solution_target->bind_rules["input.txt"] = {"input.txt", libsbox::BIND_COPY_IN};
    solution_target->bind_rules["output.txt"] = {"output.txt", libsbox::BIND_COPY_OUT};
//    solution_target->bind_rules["solution"] = {"solution", 0};

    solution_target->stdin.freopen("input.txt");
    solution_target->stdout.freopen("output.txt");

    solution_target->time_limit = 1000;
    solution_target->memory_limit = 256 * 1024;

    std::unique_ptr<libsbox::execution_context> context(new libsbox::execution_context());

    context->register_target(solution_target.get());

    context->wall_time_limit = 4000;

    context->run();

    using namespace std;
    cout << "exited: " << solution_target->exited << endl;
    cout << "exit_code: " << solution_target->exit_code << endl;
    cout << "signaled: " << solution_target->signaled << endl;
    cout << "term_signal: " << solution_target->term_signal << endl;
    cout << "time_limit_exceeded: " << solution_target->time_limit_exceeded << endl;
    cout << "wall_time_limit_exceeded: " << solution_target->wall_time_limit_exceeded << endl;
    cout << "oom_killed: " << solution_target->oom_killed << endl;
    cout << "time_usage: " << solution_target->time_usage << endl;
    cout << "time_usage_sys: " << solution_target->time_usage_sys << endl;
    cout << "time_usage_user: " << solution_target->time_usage_user << endl;
    cout << "wall_time_usage: " << solution_target->wall_time_usage << endl;
    cout << "memory_usage: " << solution_target->memory_usage << endl;
}
