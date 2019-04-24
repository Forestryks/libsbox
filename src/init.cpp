#include <libsbox/init.h>
#include <libsbox/die.h>
#include <libsbox/status.h>
#include <libsbox/signal.h>
#include <libsbox/fs.h>

#include <errno.h>
#include <cstring>
#include <unistd.h>
#include <sys/stat.h>

const std::string libsbox::prefix = "/var/lib/libsbox/invoker_data";
std::string libsbox::id;

void libsbox::init_credentials() {
    if (geteuid() != 0) die("Must be started as root");

    // May be unnecessary
    if (setresuid(0, 0, 0)) die("Cannot change to root user (%s)", strerror(errno));
    if (setresgid(0, 0, 0)) die("Cannot change to root group (%s)", strerror(errno));

    umask(022);
}

void libsbox::init(void (*fatal_error_handler)(const char *)) {
    fatal_handler = fatal_error_handler;
    if (initialized) die("Already initialized");

    init_credentials();
    disable_signals();
    id = make_temp_dir(prefix);
    std::string path = join_path(prefix, id);
    if (chdir(path.c_str())) {
        die("Cannot chdir to %s (%s)", path.c_str(), strerror(errno));
    }

    initialized = true;
}
