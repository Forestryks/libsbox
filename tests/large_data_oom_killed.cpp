/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include <libsbox.h>
#include <string>
#include <map>
#include <cassert>
#include <iostream>
#include <memory>
#include <unistd.h>
#include <cstring>

int target_main(int, char *[]) {
    // not used
    return 0;
}

int invoke_main(int argc, char *argv[]) {
    libsbox::init([](const char *msg){
        std::cout << "Error: " << msg << std::endl;
    });

    assert(argc >= 3);

    std::vector<std::string> arg;
    arg.emplace_back(argv[2]);

    std::unique_ptr<libsbox::execution_target> target(new libsbox::execution_target(arg));
    std::unique_ptr<libsbox::execution_context> context(new libsbox::execution_context());

    target->add_standard_rules();

    target->memory_limit = 1024; // 1MB
    target->time_limit = 200;
    context->wall_time_limit = 200;

    context->register_target(target.get());
    context->run();

    std::cout << "exited: " << target->exited << std::endl;
    std::cout << "exit_code: " << target->exit_code << std::endl;
    std::cout << "signaled: " << target->signaled << std::endl;
    std::cout << "term_signal: " << target->term_signal << std::endl;
    std::cout << "time_limit_exceeded: " << target->time_limit_exceeded << std::endl;
    std::cout << "wall_time_limit_exceeded: " << target->wall_time_limit_exceeded << std::endl;
    std::cout << "oom_killed: " << target->oom_killed << std::endl;
    std::cout << "time_usage: " << target->time_usage << std::endl;
    std::cout << "time_usage_sys: " << target->time_usage_sys << std::endl;
    std::cout << "time_usage_user: " << target->time_usage_user << std::endl;
    std::cout << "wall_time_usage: " << target->wall_time_usage << std::endl;
    std::cout << "memory_usage: " << target->memory_usage << std::endl;

//    assert(target->exited);
//    assert(target->exit_code == -1);
//    assert(target->signaled);
//    assert(target->term_signal == term_signal);
    assert(!target->time_limit_exceeded);
    assert(!target->wall_time_limit_exceeded);
    assert(target->oom_killed);
//    assert(target->time_usage);
//    assert(target->time_usage_sys);
//    assert(target->time_usage_user);
//    assert(target->wall_time_usage);
//    assert(target->memory_usage);

    return 0;
}

int main(int argc, char *argv[]) {
    assert(argc >= 2);
    std::string target = argv[1];
    if (target == "invoke") {
        return invoke_main(argc, argv);
    } else {
        return target_main(argc, argv);
    }
}
