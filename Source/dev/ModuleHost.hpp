#pragma once

#include "dev/GameBuild.hpp"

#include <atomic>
#include <deque>
#include <functional>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

#include <sys/types.h>

namespace cinder::core { class GameModule; }
namespace cinder::scene { class Scene; }

namespace cinder::dev {

struct BuildResult {
    bool ok = false;
    bool linked = false;
    int compiled = 0;
    int errors = 0;
};

class ModuleHost {
public:
    ModuleHost(cinder::scene::Scene& scene, std::filesystem::path project, std::string name,
               std::optional<GameBuild> build);
    ~ModuleHost();

    ModuleHost(const ModuleHost&) = delete;
    ModuleHost& operator=(const ModuleHost&) = delete;

    bool available() const { return build_.has_value(); }
    bool hasSources() const;
    bool building() const { return building_.load(); }
    bool reloadPending() const { return reloadPending_; }
    const BuildResult& lastResult() const { return last_; }
    const cinder::core::GameModule* module() const { return module_.get(); }
    std::filesystem::path library() const;
    std::string status() const;

    bool buildNow();
    bool requestBuild();
    void cancel();
    void update();
    bool reload();

private:
    BuildResult run(const std::function<void(const std::string&)>& sink);
    bool swap();
    void join();

    cinder::scene::Scene& scene_;
    std::filesystem::path project_;
    std::string name_;
    std::optional<GameBuild> build_;
    std::unique_ptr<cinder::core::GameModule> module_;
    std::thread worker_;
    std::atomic<bool> building_ = false;
    std::mutex mutex_;
    std::deque<std::string> lines_;
    pid_t pid_ = 0;
    BuildResult pending_;
    BuildResult last_;
    bool reported_ = true;
    bool reloadPending_ = false;
    int generation_ = 0;
};

}
