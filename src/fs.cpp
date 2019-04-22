#include <libsbox/fs.h>
#include <libsbox/die.h>

#include <algorithm>
#include <cstring>
#include <unistd.h>
#include <ftw.h>

std::string libsbox::join_path(const std::string &path) {
    return path;
}

bool libsbox::dir_exists(const std::string &path)  {
    struct stat st = {};
    return (stat(path.c_str(), &st) >= 0 && S_ISDIR(st.st_mode));
}

void libsbox::make_path(std::string path, int rules = 0755) {
    if (path.empty()) die("make_path() with empty path");

    auto iter = path.begin();
    if (*iter == '/') iter++;

    while (iter < path.end()) {
        iter = std::find(iter, path.end(), '/');
        if (iter != path.end()) *iter = 0;

        if (mkdir(path.c_str(), rules) < 0 && errno != EEXIST) {
            die("Cannot create directory %s (%s)", path.c_str(), strerror(errno));
        }

        if (iter == path.end()) break;
        *iter = '/';
        iter++;
    }

    if (!dir_exists(path)) {
        die("Cannot create directory %s: directory not created", path.c_str());
    }
}

namespace libsbox {
    static bool strict;

    static int rmtree_handler(const char *path, const struct stat *st, int tflag, struct FTW *ftwbuf) {
        if (S_ISDIR(st->st_mode)) {
            if (rmdir(path) < 0 && strict) {
                die("Cannot remove directory %s (%s)", path, strerror(errno));
            }
        } else {
            if (unlink(path) < 0 && strict) {
                die("Cannot remove file %s (%s)", path, strerror(errno));
            }
        }
        return 0;
    }
}

int libsbox::rmtree(const char *path, bool is_strict = true) {
    strict = is_strict;
    return nftw(path, rmtree_handler, 20, FTW_MOUNT | FTW_PHYS | FTW_DEPTH);
}
