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

void inf_rec(int x) {
    inf_rec(x + 1);
}

int target_main(int, char *[]) {
    inf_rec(0);
    return 0;
}

int invoke_main(int argc, char *argv[]) {
    libsbox::init();

    assert(argc >= 2);

    std::vector<std::string> arg;
    arg.emplace_back(argv[0]);
    arg.emplace_back("target");

    std::unique_ptr<libsbox::execution_target> target(new libsbox::execution_target(arg));
    std::unique_ptr<libsbox::execution_context> context(new libsbox::execution_context());

    target->memory_limit = 2048; // 1MB

    context->register_target(target.get());
    context->run();

    std::cout << target->plain_results() << std::endl;

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
