/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include "testing.h"

#include <time.h>

static int invoker_main(const std::vector<std::string> &args) {
    int target_time_usage = stoi(args[0]);
    int allowed_delta = stoi(args[1]);
    GenericTarget target = GenericTarget::from_current_executable("target", args[0]);
    target.set_time_limit_ms(target_time_usage * 2);
    Testing::safe_run({&target});
    target.print_stats(std::cerr);
    assert(target.exited());
    assert(labs(target.get_time_usage_ms() - target_time_usage) <= allowed_delta);
    return 0;
}

static int target_main(const std::vector<std::string> &args) {
    double target_time_usage = static_cast<double>(stoi(args[0])) / 1000;
    while (static_cast<double>(clock()) / CLOCKS_PER_SEC < target_time_usage) {
        asm volatile ("" : : : "memory");
    }
    return 0;
}

int main(int argc, char *argv[]) {
    Testing::add_handler("invoker", invoker_main);
    Testing::add_handler("target", target_main);
    Testing::start(argc, argv);
}
