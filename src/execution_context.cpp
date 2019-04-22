#include <libsbox/execution_context.h>
#include <libsbox/die.h>
#include <libsbox/status.h>

libsbox::execution_context::execution_context() {
    if (!initialized) {
        die("libsbox is not initialized");
    }
    if (!registered_contexts.insert(this).second) {
        die("Failed to register context");
    }
}

libsbox::execution_context::~execution_context() {
    if (registered_contexts.erase(this) == 0) {
        die("Failed to unregister context");
    }
}
