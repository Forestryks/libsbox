#ifndef LIBSBOX_FS_H
#define LIBSBOX_FS_H

#include <string>
#include <sys/stat.h>

namespace libsbox {
    std::string join_path(const std::string &);

    template<typename... types>
    std::string join_path(const std::string &path1, types... paths) {
        std::string path2 = join_path(paths...);
        if (path1.back() == '/') return path1 + path2;
        return path1 + "/" + path2;
    }

    bool dir_exists(const std::string &);
    void make_path(std::string, int rules = 0755);
    int rmtree(const std::string &, bool is_strict = true);
    std::string make_temp_dir(const std::string &, int rules = 0755);
} // namespace libsbox

#endif //LIBSBOX_FS_H
