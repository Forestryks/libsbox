/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_IO_H
#define LIBSBOX_IO_H

#include <string>
#include <vector>

namespace libsbox {
    class io_stream;

    class in_stream;

    class out_stream;

    struct io_pipe {
        out_stream *write_end;
        in_stream *read_end;
        int extra_flags;
    };

    struct io_infile {
        std::string filename;
        in_stream *stream;
        int extra_flags;
    };

    struct io_outfile {
        out_stream *stream;
        std::string filename;
        int extra_flags;
    };

    extern std::vector<io_pipe> pipes;
    extern std::vector<io_infile> in_streams;
    extern std::vector<io_outfile> out_streams;

    void link(out_stream &, in_stream &, int pipe_flags = 0);

    void link(const std::string &, in_stream &, int open_flags = 0);

    void link(out_stream &, const std::string &, int open_flags = 0);
} // namespace libsbox

class libsbox::io_stream {
private:
    int fd = -1;

//    friend void link(out_stream &, in_stream &, int);
//    friend void link(const std::string &, in_stream &, int);
//    friend void link(out_stream &, const std::string &, int);
    friend class execution_target;
};

class libsbox::in_stream : libsbox::io_stream {
};

class libsbox::out_stream : libsbox::io_stream {
};

#endif //LIBSBOX_IO_H
