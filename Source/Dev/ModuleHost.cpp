#include "Dev/ModuleHost.hpp"

#include "Core/GameModule.hpp"
#include "Dev/Process.hpp"
#include "Platform/Log.hpp"
#include "Scene/Scene.hpp"
#include "Serial/SceneCodec.hpp"

#include <functional>
#include <system_error>
#include <utility>

namespace cinder::Dev {
namespace {

void tally(BuildResult& result, const std::string& line) {
    if (line.find("] compile ") != std::string::npos) result.compiled++;
    if (line.find("] link ") != std::string::npos) result.linked = true;
    if (line.find(": error:") != std::string::npos || line.find(": fatal error:") != std::string::npos) result.errors++;
}

}

ModuleHost::ModuleHost(cinder::Scene::Scene& scene, std::filesystem::path project, std::string name,
                       std::optional<GameBuild> build)
    : scene_(scene), project_(std::move(project)), name_(std::move(name)), build_(std::move(build)) {
    std::error_code error;
    std::filesystem::remove_all(project_ / "Saved" / "Binaries" / "Loaded", error);
}

ModuleHost::~ModuleHost() {
    job_.cancel();
    job_.join();
    if (module_) {
        scene_.clear();
        const std::filesystem::path loaded = module_->path();
        module_.reset();
        std::error_code error;
        std::filesystem::remove(loaded, error);
    }
}

std::filesystem::path ModuleHost::library() const {
    return project_ / "Saved" / "Binaries" / (name_ + std::string(cinder::Core::GameModule::extension()));
}

bool ModuleHost::hasSources() const { return hasGameSources(project_); }

std::string ModuleHost::status() const {
    if (building()) return "Building...";
    if (!build_ || (!last_.ok && last_.errors == 0 && module_ == nullptr)) return "";
    if (!last_.ok) return std::to_string(last_.errors) + (last_.errors == 1 ? " error" : " errors");
    if (reloadPending_) return "Reload after Stop";
    return module_ != nullptr ? "Compiled" : "";
}

void ModuleHost::cancel() { job_.cancel(); }

BuildResult ModuleHost::run(const std::function<void(const std::string&)>& sink) {
    BuildResult result;
    std::string error;
    std::unique_ptr<Process> process = Process::start(build_->game(project_), error);
    if (process == nullptr) {
        sink(error);
        result.errors = 1;
        return result;
    }
    std::string line;
    while (process->readLine(line)) {
        tally(result, line);
        sink(line);
    }
    result.ok = process->wait() == 0;
    if (!result.ok && result.errors == 0) result.errors = 1;
    return result;
}

bool ModuleHost::buildNow() {
    if (!build_ || building()) return false;
    job_.join();
    last_ = run([](const std::string& line) { cinder::Platform::logInfo("[build] %s\n", line.c_str()); });
    if (!last_.ok || !std::filesystem::exists(library())) return false;
    if (module_ != nullptr && !last_.linked) return true;
    return swap();
}

bool ModuleHost::requestBuild() {
    if (!build_ || building()) return false;
    job_.join();
    std::string error;
    std::unique_ptr<Process> process = Process::start(build_->game(project_), error);
    if (process == nullptr) {
        cinder::Platform::logError("[build] %s\n", error.c_str());
        last_ = BuildResult{false, false, 0, 1};
        return false;
    }
    pending_ = BuildResult{};
    reported_ = false;
    job_.start(std::move(process));
    return true;
}

void ModuleHost::update() {
    const auto drain = [this] {
        for (const std::string& line : job_.takeLines()) {
            tally(pending_, line);
            cinder::Platform::logInfo("[build] %s\n", line.c_str());
        }
    };
    drain();
    if (building() || reported_) return;
    job_.join();
    drain();
    reported_ = true;
    pending_.ok = job_.status() == 0;
    if (!pending_.ok && pending_.errors == 0) pending_.errors = 1;
    last_ = pending_;
    if (last_.ok && (last_.linked || module_ == nullptr) && std::filesystem::exists(library())) {
        reloadPending_ = true;
    }
}

bool ModuleHost::reload() {
    if (!reloadPending_) return false;
    reloadPending_ = false;
    return swap();
}

bool ModuleHost::swap() {
    const std::filesystem::path library = this->library();
    const std::filesystem::path loaded = project_ / "Saved" / "Binaries" / "Loaded"
            / (library.stem().string() + "-" + std::to_string(++generation_) + library.extension().string());
    std::error_code error;
    std::filesystem::create_directories(loaded.parent_path(), error);
    std::filesystem::copy_file(library, loaded, std::filesystem::copy_options::overwrite_existing, error);
    if (error) {
        cinder::Platform::logError("[module] could not stage %s: %s\n", library.string().c_str(), error.message().c_str());
        return false;
    }

    std::string reason;
    std::unique_ptr<cinder::Core::GameModule> next = cinder::Core::GameModule::open(loaded, scene_.types(), reason);
    if (next == nullptr) {
        cinder::Platform::logError("[module] %s\n", reason.c_str());
        std::filesystem::remove(loaded, error);
        return false;
    }

    const std::string text = cinder::Serial::SceneCodec::save(scene_);
    scene_.clear();
    std::filesystem::path previous;
    if (module_ != nullptr) previous = module_->path();
    module_.reset();
    if (!previous.empty()) std::filesystem::remove(previous, error);

    next->enlist();
    module_ = std::move(next);
    cinder::Serial::SceneCodec::load(text, scene_);
    cinder::Platform::logInfo("[module] loaded %s (%zu classes)\n", library.filename().string().c_str(),
                              module_->classes().size());
    return true;
}

}
