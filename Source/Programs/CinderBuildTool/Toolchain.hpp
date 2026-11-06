#pragma once

#include <string>
#include <vector>

namespace cbt {

struct Toolchain {
    std::string platform;
    std::string cxx;
    std::string cc;
    std::string identity;
    std::vector<std::string> sysroot;

    static Toolchain detect();

    std::string libraryExtension() const { return platform == "macOS" ? ".dylib" : ".so"; }
};

}
