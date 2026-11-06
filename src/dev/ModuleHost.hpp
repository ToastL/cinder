#pragma once

#include "dev/ModuleBuilder.hpp"
#include "dev/Toolchain.hpp"

#include <atomic>
#include <deque>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

namespace cinder::core { class GameModule; }
namespace cinder::scene { class Scene; }

namespace cinder::dev {

class ModuleHost {
public:
    ModuleHost(cinder::scene::Scene& scene, std::filesystem::path project, std::string name,
               std::optional<Toolchain> toolchain);
    ~ModuleHost();

    ModuleHost(const ModuleHost&) = delete;
    ModuleHost& operator=(const ModuleHost&) = delete;

    bool available() const { return builder_ != nullptr; }
    bool hasSources() const;
    bool building() const { return building_.load(); }
    bool reloadPending() const { return reloadPending_; }
    const ModuleBuilder::Result& lastResult() const { return last_; }
    const cinder::core::GameModule* module() const { return module_.get(); }
    std::string status() const;

    bool buildNow();
    bool requestBuild();
    void update();
    bool reload();

private:
    bool swap();
    void join();

    cinder::scene::Scene& scene_;
    std::filesystem::path project_;
    std::unique_ptr<ModuleBuilder> builder_;
    std::unique_ptr<cinder::core::GameModule> module_;
    std::thread worker_;
    std::atomic<bool> building_ = false;
    std::mutex mutex_;
    std::deque<std::string> lines_;
    ModuleBuilder::Result pending_;
    ModuleBuilder::Result last_;
    bool reported_ = true;
    bool reloadPending_ = false;
    int generation_ = 0;
};

}
