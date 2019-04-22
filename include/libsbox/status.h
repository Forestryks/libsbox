#ifndef LIBSBOX_STATUS_H
#define LIBSBOX_STATUS_H

#include <libsbox/execution_context.h>

#include <set>

namespace libsbox {
    extern std::set<execution_context *> registered_contexts;
    extern bool initialized;
} // namespace libsbox

#endif //LIBSBOX_STATUS_H
