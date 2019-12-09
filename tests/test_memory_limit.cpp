/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include "testing.h"

#include <time.h>

static int invoker_main(const std::vector<std::string> &args) {
    GenericTarget target = GenericTarget::from_current_executable("target", args[0]);
    target.set_memory_limit_kb(stoi(args[0]));
    Testing::safe_run({&target});
    target.print_stats(std::cerr);
    assert(!target.is_time_limit_exceeded());
    assert(!target.is_wall_time_limit_exceeded());
    assert(target.is_memory_limit_hit() || target.is_oom_killed());
    return 0;
}

static int target_main(const std::vector<std::string> &args) {
    size_t memory_limit_kb = static_cast<size_t>(stoi(args[0])) * 1024;
    char *mem = static_cast<char *>(malloc(memory_limit_kb));
    memset(mem, 1, memory_limit_kb);
    for (size_t i = 0; i < memory_limit_kb; ++i) {
        mem[i] = i;
    }
    asm volatile ("" : : : "memory");
    return 0;
}

int main(int argc, char *argv[]) {
    Testing::add_handler("invoker", invoker_main);
    Testing::add_handler("target", target_main);
    Testing::start(argc, argv);
}
