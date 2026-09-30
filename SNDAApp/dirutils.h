// Portable directory-creation helper (Qt-free).
#pragma once

#include <cerrno>
#include <string>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

namespace dirutils {

// Creates a directory and any missing parents (like `mkdir -p`).
// Returns true if the directory exists when the call finishes.
inline bool makeDirs(const std::string &path) {
    if (path.empty()) {
        return false;
    }
    std::string current;
    current.reserve(path.size());
    for (size_t i = 0; i < path.size(); ++i) {
        char c = path[i];
        current += c;
        if ((c == '/' || c == '\\') && current.size() > 1) {
            // Skip redundant separators and the root itself.
            std::string sub = current.substr(0, current.size() - 1);
            if (sub.empty() || sub == "/" || (sub.size() == 2 && sub[1] == ':')) {
                continue;
            }
#ifdef _WIN32
            _mkdir(sub.c_str());
#else
            mkdir(sub.c_str(), 0755);
#endif
        }
    }
    std::string finalPath = path;
    while (finalPath.size() > 1 &&
           (finalPath[finalPath.size() - 1] == '/' || finalPath[finalPath.size() - 1] == '\\')) {
        finalPath.erase(finalPath.size() - 1);
    }
#ifdef _WIN32
    return _mkdir(finalPath.c_str()) == 0 || errno == EEXIST;
#else
    return mkdir(finalPath.c_str(), 0755) == 0 || errno == EEXIST;
#endif
}

} // namespace dirutils
