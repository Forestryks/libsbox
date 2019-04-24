#ifndef LIBSBOX_INIT_H
#define LIBSBOX_INIT_H

#include <string>

namespace libsbox {
    extern const std::string prefix;
    extern std::string id;

    void init_credentials();
    void init(void (*fatal_error_handler)(const char *));
} // namespace libsbox

#endif //LIBSBOX_INIT_H
