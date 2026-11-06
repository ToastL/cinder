#include "CinderBuildTool/Toolchain.hpp"

#include "CinderBuildTool/Process.hpp"

#include <cctype>
#include <cstdlib>
#include <stdexcept>

namespace cbt {
namespace {

std::string firstLine(const std::vector<std::string>& argv) {
    std::string error;
    std::unique_ptr<Process> process = Process::start(argv, error);
    if (process == nullptr) throw std::runtime_error(error);
    std::string line;
    std::string first;
    while (process->readLine(line)) {
        if (first.empty()) first = line;
    }
    if (process->wait() != 0) throw std::runtime_error(argv[0] + " failed");
    return first;
}

std::string identityOf(const std::string& compiler) {
    std::string version = firstLine({compiler, "--version"});
    std::string out;
    for (const char c : version) {
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '.') out.push_back(c);
        else if (!out.empty() && out.back() != '-') out.push_back('-');
    }
    while (!out.empty() && out.back() == '-') out.pop_back();
    return out;
}

}

Toolchain Toolchain::detect() {
    Toolchain toolchain;
#if defined(__APPLE__)
    toolchain.platform = "macOS";
    toolchain.cxx = std::getenv("CXX") != nullptr ? std::getenv("CXX") : firstLine({"xcrun", "--find", "clang++"});
    toolchain.cc = std::getenv("CC") != nullptr ? std::getenv("CC") : firstLine({"xcrun", "--find", "clang"});
    toolchain.sysroot = {"-isysroot", firstLine({"xcrun", "--show-sdk-path"})};
#else
    toolchain.platform = "Linux";
    toolchain.cxx = std::getenv("CXX") != nullptr ? std::getenv("CXX") : "c++";
    toolchain.cc = std::getenv("CC") != nullptr ? std::getenv("CC") : "cc";
#endif
    toolchain.identity = identityOf(toolchain.cxx);
    return toolchain;
}

}
