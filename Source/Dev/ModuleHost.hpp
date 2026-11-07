#pragma once

#include "Dev/BackgroundProcess.hpp"
#include "Dev/GameBuild.hpp"

#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace cinder::Core { class GameModule; }
namespace cinder::Scene { class Scene; }

namespace cinder::Dev {

struct BuildResult {
    bool ok = false;
    bool linked = false;
    int compiled = 0;
    int errors = 0;
};

class ModuleHost {
public:
    ModuleHost(cinder::Scene::Scene& scene, std::filesystem::path project, std::string name,
               std::optional<GameBuild> build);
    ~ModuleHost();

    ModuleHost(const ModuleHost&) = delete;
    ModuleHost& operator=(const ModuleHost&) = delete;

    bool available() const { return build_.has_value(); }
    bool hasSources() const;
    bool building() const { return job_.running(); }
    bool reloadPending() const { return reloadPending_; }
    const BuildResult& lastResult() const { return last_; }
    const cinder::Core::GameModule* module() const { return module_.get(); }
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

    cinder::Scene::Scene& scene_;
    std::filesystem::path project_;
    std::string name_;
    std::optional<GameBuild> build_;
    std::unique_ptr<cinder::Core::GameModule> module_;
    BackgroundProcess job_;
    BuildResult pending_;
    BuildResult last_;
    bool reported_ = true;
    bool reloadPending_ = false;
    int generation_ = 0;
};

}
