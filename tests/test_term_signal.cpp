/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include "testing.h"
#include <signal.h>

static int invoker_main(const std::vector<std::string> &args) {
    assert(args[1] == "terminated" || args[1] == "exited");
    GenericTarget target = GenericTarget::from_current_executable("target", args[0]);
    Testing::safe_run({&target});
    target.print_stats(std::cerr);
    if (args[1] == "exited") {
        target.assert_exited(0);
    } else {
        target.assert_killed(stoi(args[0]));
    }
    return 0;
}

static int target_main(const std::vector<std::string> &args) {
    raise(stoi(args[0]));
    return 0;
}

int main(int argc, char *argv[]) {
    Testing::add_handler("invoker", invoker_main);
    Testing::add_handler("target", target_main);
    Testing::start(argc, argv);
}
