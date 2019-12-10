/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include "testing.h"

static int invoker_main(const std::vector<std::string> &args) {
    GenericTarget target = GenericTarget::from_current_executable("target", args[0]);
    Testing::safe_run({&target});
    target.print_stats(std::cerr);
    target.assert_exited(stoi(args[0]));
    return 0;
}

static int target_main(const std::vector<std::string> &args) {
    return stoi(args[0]);
}

int main(int argc, char *argv[]) {
    Testing::add_handler("invoker", invoker_main);
    Testing::add_handler("target", target_main);
    Testing::start(argc, argv);
}
