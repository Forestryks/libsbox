/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include <libsbox.h>
#include <string>
#include <map>
#include <cassert>
#include <iostream>
#include <memory>

int target_main(int argc, char *argv[]) {
    assert(argc >= 3);
    int exit_code = strtol(argv[2], nullptr, 10);
    return exit_code;
}

int invoke_main(int argc, char *argv[]) {
    libsbox::init([](const char *msg){
        std::cout << "Error: " << msg << std::endl;
    });

    assert(argc >= 3);

    std::vector<std::string> arg;
    arg.emplace_back(argv[0]);
    arg.emplace_back("target");
    arg.emplace_back(argv[2]);

    std::unique_ptr<libsbox::execution_target> target(new libsbox::execution_target(arg));
    std::unique_ptr<libsbox::execution_context> context(new libsbox::execution_context());

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
    std::cout << "time_usage_wall: " << target->time_usage_wall << std::endl;
    std::cout << "memory_usage: " << target->memory_usage << std::endl;

    int exit_code = strtol(argv[2], nullptr, 10);

    assert(target->exited);
    assert(target->exit_code == exit_code);
    assert(!target->signaled);
    assert(target->term_signal == -1);
    assert(!target->time_limit_exceeded);
    assert(!target->wall_time_limit_exceeded);
    assert(!target->oom_killed);
//    assert(target->time_usage);
//    assert(target->time_usage_sys);
//    assert(target->time_usage_user);
//    assert(target->time_usage_wall);
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
