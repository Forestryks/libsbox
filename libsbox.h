#ifndef LIBSBOX_LIBSBOX_H
#define LIBSBOX_LIBSBOX_H

#include <string>

namespace libsbox {

int a, b, c;
const long R_COPY_TO_BOX = 1;
const long R_COPY_FROM_BOX = 2;

class execution_metadata {
public:
    int time_usage;
    int memory_usage;
    int exit_code;
};

class execution_target {
public:
    int stdin, stdout;
    void add_path_rule(std::string outside, std::string inside, long flags, int mode = 755) {

    }

    void set_time_limit(long limit) {

    }

    void set_memory_limit(long limit) {

    }

    execution_metadata *get_execution_metadata() {
        return new execution_metadata();
    }
};


class execution_context {
public:
    int register_target(execution_target *target) {
        return 0;
    }

    void create_pipe(int a, int b, int c) {
    }

    void set_wall_time_limit(long limit) {

    }

    void run() {

    }
};

} // namespace libsbox

#endif //LIBSBOX_LIBSBOX_H
