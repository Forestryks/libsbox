#include <libsbox/init.h>
#include <libsbox/die.h>
#include <libsbox/status.h>

#include <errno.h>
#include <cstring>
#include <unistd.h>
#include <sys/stat.h>

void libsbox::init_credentials() {
    if (geteuid() != 0) die("Must be started as root");

    // May be unnecessary
    if (setresuid(0, 0, 0)) die("Cannot change to root user (%s)", strerror(errno));
    if (setresgid(0, 0, 0)) die("Cannot change to root group (%s)", strerror(errno));

    umask(022);
}

void libsbox::init() {
    if (initialized) die("Already initialized");
    init_credentials();
    initialized = true;
}
