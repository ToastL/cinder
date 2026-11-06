#include "dev/ModuleHost.hpp"

#include "core/GameModule.hpp"
#include "dev/Process.hpp"
#include "platform/Log.hpp"
#include "scene/Scene.hpp"
#include "serial/SceneCodec.hpp"

#include <csignal>
#include <functional>
#include <system_error>
#include <utility>

namespace cinder::dev {

ModuleHost::ModuleHost(cinder::scene::Scene& scene, std::filesystem::path project, std::string name,
                       std::optional<GameBuild> build)
    : scene_(scene), project_(std::move(project)), name_(std::move(name)), build_(std::move(build)) {
    std::error_code error;
    std::filesystem::remove_all(project_ / "Saved" / "Binaries" / "Loaded", error);
}

ModuleHost::~ModuleHost() {
    cancel();
    join();
    if (module_) {
        scene_.clear();
        const std::filesystem::path loaded = module_->path();
        module_.reset();
        std::error_code error;
        std::filesystem::remove(loaded, error);
    }
}

std::filesystem::path ModuleHost::library() const {
    return project_ / "Saved" / "Binaries" / (name_ + std::string(cinder::core::GameModule::extension()));
}

bool ModuleHost::hasSources() const {
    std::error_code error;
    for (auto it = std::filesystem::recursive_directory_iterator(project_ / "Source", error);
         !error && it != std::filesystem::recursive_directory_iterator(); it.increment(error)) {
        if (it->is_regular_file() && it->path().extension() == ".cpp") return true;
    }
    return false;
}

std::string ModuleHost::status() const {
    if (building()) return "Building...";
    if (!build_ || (!last_.ok && last_.errors == 0 && module_ == nullptr)) return "";
    if (!last_.ok) return std::to_string(last_.errors) + (last_.errors == 1 ? " error" : " errors");
    if (reloadPending_) return "Reload after Stop";
    return module_ != nullptr ? "Compiled" : "";
}

void ModuleHost::join() {
    if (worker_.joinable()) worker_.join();
}

void ModuleHost::cancel() {
    const std::lock_guard lock(mutex_);
    if (pid_ > 0) kill(-pid_, SIGTERM);
}

BuildResult ModuleHost::run(const std::function<void(const std::string&)>& sink) {
    BuildResult result;
    std::string error;
    std::unique_ptr<Process> process = Process::start(build_->game(project_), error);
    if (process == nullptr) {
        sink(error);
        result.errors = 1;
        return result;
    }
    {
        const std::lock_guard lock(mutex_);
        pid_ = process->pid();
    }
    std::string line;
    while (process->readLine(line)) {
        if (line.find("] compile ") != std::string::npos) result.compiled++;
        if (line.find("] link ") != std::string::npos) result.linked = true;
        if (line.find(": error:") != std::string::npos || line.find(": fatal error:") != std::string::npos) result.errors++;
        sink(line);
    }
    {
        const std::lock_guard lock(mutex_);
        pid_ = 0;
    }
    result.ok = process->wait() == 0;
    if (!result.ok && result.errors == 0) result.errors = 1;
    return result;
}

bool ModuleHost::buildNow() {
    if (!build_ || building()) return false;
    join();
    last_ = run([](const std::string& line) { cinder::platform::logInfo("[build] %s\n", line.c_str()); });
    if (!last_.ok || !std::filesystem::exists(library())) return false;
    if (module_ != nullptr && !last_.linked) return true;
    return swap();
}

bool ModuleHost::requestBuild() {
    if (!build_ || building()) return false;
    join();
    building_.store(true);
    reported_ = false;
    worker_ = std::thread([this] {
        BuildResult result = run([this](const std::string& line) {
            const std::lock_guard lock(mutex_);
            lines_.push_back(line);
        });
        {
            const std::lock_guard lock(mutex_);
            pending_ = result;
        }
        building_.store(false);
    });
    return true;
}

void ModuleHost::update() {
    std::deque<std::string> lines;
    {
        const std::lock_guard lock(mutex_);
        lines.swap(lines_);
    }
    for (const std::string& line : lines) cinder::platform::logInfo("[build] %s\n", line.c_str());
    if (building() || reported_) return;

    join();
    reported_ = true;
    {
        const std::lock_guard lock(mutex_);
        last_ = pending_;
    }
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
        cinder::platform::logError("[module] could not stage %s: %s\n", library.string().c_str(), error.message().c_str());
        return false;
    }

    std::string reason;
    std::unique_ptr<cinder::core::GameModule> next = cinder::core::GameModule::open(loaded, scene_.types(), reason);
    if (next == nullptr) {
        cinder::platform::logError("[module] %s\n", reason.c_str());
        std::filesystem::remove(loaded, error);
        return false;
    }

    const std::string text = cinder::serial::SceneCodec::save(scene_);
    scene_.clear();
    std::filesystem::path previous;
    if (module_ != nullptr) previous = module_->path();
    module_.reset();
    if (!previous.empty()) std::filesystem::remove(previous, error);

    next->enlist();
    module_ = std::move(next);
    cinder::serial::SceneCodec::load(text, scene_);
    cinder::platform::logInfo("[module] loaded %s (%zu classes)\n", library.filename().string().c_str(),
                              module_->classes().size());
    return true;
}

}
