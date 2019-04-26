/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include <libsbox/io.h>

/*
 * Links live only for one execution. That is need to be recreated every time
 */

namespace libsbox {
    std::vector<io_pipe> pipes;
    std::vector<io_infile> in_streams;
    std::vector<io_outfile> out_streams;
} // namespace libsbox

void libsbox::link(libsbox::out_stream &write_end, libsbox::in_stream &read_end, int pipe_flags) {
    pipes.push_back({&write_end, &read_end, pipe_flags});
}

void libsbox::link(const std::string &filename, libsbox::in_stream &stream, int open_flags) {
    in_streams.push_back({filename, &stream, open_flags});
}

void libsbox::link(libsbox::out_stream &stream, const std::string &filename, int open_flags) {
    out_streams.push_back({&stream, filename, open_flags});
}
