/* 
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_H_
#define LIBSBOX_H_

#include <iostream>
#include <random>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <stdarg.h>
#include <algorithm>
#include <ftw.h>

namespace libsbox {

std::string boxes_prefix = "/var/lib/libsbox/boxes/";

int box_id_len = 20;
const std::string id_chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
std::string box_id;
std::string box_dir;

enum box_states {
    BOX_INVOKER,
    BOX_PROXY,
    BOX_TARGET
};
int box_state = BOX_INVOKER;

void invoker_die(char *);
void proxy_die(char *);
void target_die(char *);
void die(const char *, ...);

std::string merge_paths() {
    return "";
}

template <typename T, typename... Types>
std::string merge_paths(const T &path1, Types... rpath2) {
    std::string path2 = merge_paths(rpath2...);
    if (!path2.empty() && (path1.empty() || path1.back() != '/')) return path1 + "/" + path2;
    return path1 + path2;
}

/*
 * Check if dir exists
 */
bool dir_exists(std::string path) {
    struct stat st;
    return (stat(path.c_str(), &st) >= 0 && S_ISDIR(st.st_mode));
}

/*
 * Create all directories on path
 */
void make_path(std::string path, int rules = 0755) {
    if (path.empty()) die("make_path() with empty path");

    auto iter = path.begin();
    if (*iter == '/') iter++;

    while (iter < path.end()) {
        iter = std::find(iter, path.end(), '/');
        if (iter != path.end()) *iter = 0;

        if (mkdir(path.c_str(), rules) < 0 && errno != EEXIST)
            die("Cannot create directory %s (%s)", path.c_str(), strerror(errno));

        if (iter == path.end()) break;
        *iter = '/';
        iter++;
    }

    if (!dir_exists(path))
        die("Cannot create directory %s: directory not exists", path.c_str());
}

/*
 * nftw traversal handler
 */
bool strict;
int rmtree_handler(const char *path, const struct stat *st, int tflag, struct FTW *ftwbuf) {
    if (S_ISDIR(st->st_mode)) {
        if (rmdir(path) < 0 && strict)
            die("Cannot remove directory %s (%s)", path, strerror(errno));
    } else {
        if (unlink(path) < 0 && strict)
            die("Cannot remove file %s (%s)", path, strerror(errno));
    }
    return 0;
}

/*
 * Removes dir using nftw traversal
 */
int rmtree(const char *path, bool is_strict = true) {
    strict = is_strict;
    return nftw(path, rmtree_util, 20, FTW_MOUNT | FTW_PHYS | FTW_DEPTH);
}

namespace cgroup {

std::string root = "/sys/fs/cgroup/";
std::string name;

struct controller {
    std::string name;
};

struct controller controllers[] = {
    {"memory"},
    {"cpuacct"},
    // {"cpuset"}
};

/*
 * Initialize cgroup filesystem
 */
void init() {
    if (!dir_exists(root))
        die("cgroup filesystem is not mounted at %s", root.c_str());

    name = "sbox-" + box_id;

    struct stat st;
    for (auto &controller : controllers) {
        std::string path = merge_paths(root, controller.name, name);
        if ((stat(path.c_str(), &st) >= 0 || errno != ENOENT) && rmdir(path.c_str()) < 0)
            die("Cannot remove existing cgroup controller %s (%s)", path.c_str(), strerror(errno));
        if (mkdir(path.c_str(), 0755) < 0)
            die("Cannot create cgroup controller %s (%s)", path.c_str(), strerror(errno));
    }
}

/*
 * Somithing went wrong. Remove cgroup controllers without error checks
 */
void die() {
    for (auto &controller : controllers) {
        std::string path = merge_paths(root, controller.name, name);
        rmdir(path.c_str());
    }
}

} // namespace cgroup

/*
 * Check that started as root, change to root user and group and set umask to 022
 */
void init_credentials() {
    if (geteuid() != 0) die("Must be started as root");

    // May be unnecessary
    if (setresuid(0, 0, 0)) die("Cannot change to root user (%s)", strerror(errno));
    if (setresgid(0, 0, 0)) die("Cannot change to root group (%s)", strerror(errno));

    umask(022);
}

/*
 * Generate unique box_id
 */
void generate_id() {
    std::random_device rd;
    std::mt19937 mt(rd());

    int ret;
    do {
        box_id.assign(box_id_len, '-');
        for (int i = 0; i < box_id_len; ++i) {
            box_id[i] = id_chars[mt() % id_chars.size()];
        }
        box_dir = merge_paths(boxes_prefix, box_id);

        struct stat st;
        ret = stat(box_dir.c_str(), &st);
        if (ret != 0 && errno != ENOENT) {
            die("Cannot initialize sandbox: failed to assign id (%s)", strerror(errno));
        }
    } while (ret == 0);
}

/*
 * Create box, chdir to it
 */
void init_box() {
    if (dir_exists(box_dir))
        die("Directory %s already exists", box_dir.c_str());
    make_path(box_dir);
    if (chdir(box_dir.c_str()) < 0)
        die("Cannot chdir to %s (%s)", box_dir.c_str(), strerror(errno));
}

/*
 * Initialize sandbox. Must be executed only once
 */
void init() {
    init_credentials();
    generate_id();
    init_box();
    cgroup::init();
}

/*
 * Invoker internal error
 */
void invoker_die(char *buf) {
    printf("[invoker] %s\n", buf);
    rmtree(box_dir.c_str(), false);
    cgroup::die();
    exit(-1);
}

/*
 * Proxy internal error
 */
void proxy_die(char *buf) {
    printf("[proxy] %s\n", buf);
    exit(-1);
}

/*
 * Target internal error
 */
void target_die(char *buf) {
    printf("[target] %s\n", buf);
    exit(-1);
}

/*
 * Die gracefully
 */
void die(const char *msg, ...) {
    char buf[2048];
    va_list va_args;
    va_start(va_args, msg);
    vsnprintf(buf, 2048, msg, va_args);
    va_end(va_args);

    // printf("%s\n", err_buf);
    switch (box_state) {
        case BOX_INVOKER:
            invoker_die(buf);
            break;
        case BOX_PROXY:
            proxy_die(buf);
            break;
        case BOX_TARGET:
            target_die(buf);
            break;
    }
    // we shall not get there
    exit(-1);
}

} // namespace libsbox

#endif /* #ifndef LIBSBOX_H_ */
