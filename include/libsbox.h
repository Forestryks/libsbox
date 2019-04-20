#ifndef LIBSBOX_LIBSBOX_H
#define LIBSBOX_LIBSBOX_H

#include <libsbox/fs.h>
#include <string>
#include <stdarg.h>
#include <set>

namespace libsbox {
    class execution_context {
    public:
        execution_context();
        ~execution_context();
    public:
    };

    class execution_target {
    public:
        execution_target() = default;
        ~execution_target() = default;
    public:
    };

    template<bool critical=false> void die(const char *, ...);
    extern std::set<execution_context *> registered_contexts;

} // namespace libsbox

#endif //LIBSBOX_LIBSBOX_H
