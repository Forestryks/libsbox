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

    std::unique_ptr<libsbox::execution_target> solution_target(new libsbox::execution_target({"test"}));

    libsbox::link("input.txt", solution_target->stdin);
    libsbox::link(solution_target->stdout, "output.txt");

    solution_target->bind_rules["file"] = {"file", libsbox::BIND_FILE | libsbox::BIND_READWRITE};

    solution_target->time_limit = 1000;
    solution_target->memory_limit = 256 * 1024;
    solution_target->fsize_limit = 256 * 1024;
    solution_target->max_threads = 1;


//    std::unique_ptr<libsbox::execution_context> test_context(new libsbox::execution_context());
//    std::unique_ptr<libsbox::execution_target> solution_target(new libsbox::execution_target());
//    std::unique_ptr<libsbox::execution_target> interactor_target(new libsbox::execution_target());
//    std::unique_ptr<libsbox::execution_context> interaction_context(new libsbox::execution_context());
//    interaction_context->register_target(solution_target.get());
//    interaction_context->register_target(interactor_target.get());
//
//    interaction_context->create_pipe(solution_target->stdout, interactor_target->stdin, 0);
//    interaction_context->create_pipe(interactor_target->stdout, solution_target->stdin, 0);
//
//    solution_target->set_time_limit(1000);
//    solution_target->set_memory_limit(256 * 1024);
//    interaction_context->set_wall_time_limit(2000);
//
//    interactor_target->add_path_rule("test.in", "test.in", libsbox::R_COPY_TO_BOX);
//    interactor_target->add_path_rule("test.out", "test.out", libsbox::R_COPY_FROM_BOX);
//
//    interaction_context->run();
//
//    std::unique_ptr<libsbox::execution_metadata> solution_metadata(solution_target->get_execution_metadata());
//
//    std::cout << "exit-code:" << solution_metadata->exit_code << "\n";
//    std::cout << "time-usage:" << solution_metadata->time_usage << "\n";
//    std::cout << "memory-usage:" << solution_metadata->memory_usage << "\n";
}
