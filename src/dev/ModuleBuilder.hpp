#pragma once

#include "dev/Toolchain.hpp"

#include <atomic>
#include <filesystem>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

#include <sys/types.h>

namespace cinder::dev {

class ModuleBuilder {
public:
    using Sink = std::function<void(const std::string&)>;

    struct Result {
        bool ok = false;
        bool linked = false;
        int compiled = 0;
        int errors = 0;
    };

    ModuleBuilder(std::filesystem::path project, std::string name, Toolchain toolchain,
                  std::filesystem::path library = {}, std::string config = "Editor");

    const std::filesystem::path& library() const { return library_; }
    std::vector<std::filesystem::path> sources() const;

    Result build(const Sink& sink);
    void cancel();

private:
    struct Job {
        std::filesystem::path source;
        std::filesystem::path object;
        std::filesystem::path depfile;
    };

    bool stale(const Job& job, bool rebuildAll) const;
    bool run(const std::vector<std::string>& args, std::vector<std::string>& output);
    void writeCompileCommands(const std::vector<Job>& jobs) const;

    std::filesystem::path project_;
    std::string name_;
    Toolchain toolchain_;
    std::filesystem::path library_;
    std::filesystem::path intermediate_;
    std::atomic<bool> cancelled_ = false;
    std::mutex mutex_;
    std::vector<pid_t> running_;
};

}
