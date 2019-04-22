#include <libsbox/status.h>

namespace libsbox {
    std::set<execution_context *> registered_contexts = {};
    bool initialized = false;
} // namespace libsbox
