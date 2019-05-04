/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include <libsbox/fs.h>
#include <libsbox/die.h>

#include <algorithm>
#include <cstring>
#include <unistd.h>
#include <ftw.h>
#include <random>
#include <sys/stat.h>
#include <sys/types.h>

bool libsbox::dir_exists(const std::string &path)  {
    struct stat st = {};
    return (stat(path.c_str(), &st) >= 0 && S_ISDIR(st.st_mode));
}

void libsbox::make_path(std::string path, int rules) {
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

    static int rmtree_handler(const char *path, const struct stat *st, int, struct FTW *) {
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

int libsbox::rmtree(const std::string &path, bool is_strict) {
    strict = is_strict;
    return nftw(path.c_str(), rmtree_handler, 20, FTW_MOUNT | FTW_PHYS | FTW_DEPTH);
}

const int NAME_LEN = 15;
const int NUM_TRIES = 10;
const char ALPHA[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
const int ALPHA_LEN = strlen(ALPHA);

/*
 * Returns created dir name
 */
std::string libsbox::make_temp_dir(const std::string &prefix, int rules) {
    if (prefix.empty()) die("make_temp with empty path");
    std::random_device rd;
    std::mt19937 mt(rd());

    int tries = 0;
    std::string name;
    while (tries < NUM_TRIES) {
        tries++;
        name.resize(NAME_LEN);
        for (int i = 0; i < NAME_LEN; ++i) {
            name[i] = ALPHA[mt() % ALPHA_LEN];
        }
        std::string path = join_path(prefix, name);

        struct stat st = {};

        if (stat(path.c_str(), &st) == 0 || errno != ENOENT) continue;

        make_path(path, rules);
        return name;
    }

    die("Failed to find free id within %d tries", NUM_TRIES);
}
