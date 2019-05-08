/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include <libsbox/execution_context.h>
#include <libsbox/signal.h>

void libsbox::execution_context::register_target(libsbox::execution_target *target) {
    this->targets.push_back(target);
}

void libsbox::execution_context::link(libsbox::out_stream *write_end, libsbox::in_stream *read_end, int pipe_flags) {
    this->pipes.push_back({write_end, read_end, pipe_flags});
}

void libsbox::execution_context::link(const std::string &filename, libsbox::in_stream *stream, int open_flags) {
    this->input_files.push_back({filename, stream, open_flags});
}

void libsbox::execution_context::link(libsbox::out_stream *stream, const std::string &filename, int open_flags) {
    this->output_files.push_back({stream, filename, open_flags});
}

void libsbox::execution_context::run() {
    current_context = this;

    for (auto target : this->targets) {
        target->prepare();
    }
    // create all pipes and open fds



    // for every target:
        // remove cgroup
        // remove dir
        // close all created pipes and fds
    for (auto target : this->targets) {
        target->cleanup();
    }

    current_context = nullptr;
}

void libsbox::execution_context::die() {
    for (auto target : this->targets) {
        target->die();
    }
}

namespace libsbox {
    execution_context *current_context = nullptr;
} // namespace libsbox
