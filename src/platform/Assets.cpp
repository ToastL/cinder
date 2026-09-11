#include "platform/Assets.hpp"

#include <cstdlib>
#include <system_error>

#ifndef CINDER_ENGINE_DEFAULT
#define CINDER_ENGINE_DEFAULT "engine"
#endif

namespace cinder::platform {
namespace {

std::filesystem::path exeDir;
std::filesystem::path engine;
std::filesystem::path project;
bool engineResolved = false;

std::filesystem::path resolveEngine() {
    if (const char* env = std::getenv("CINDER_ENGINE"); env != nullptr && *env != '\0') return env;

    if (!exeDir.empty() && std::filesystem::exists(exeDir / "engine")) return exeDir / "engine";

    const std::filesystem::path baked(CINDER_ENGINE_DEFAULT);
    if (std::filesystem::exists(baked)) return baked;

    return std::filesystem::path("engine");
}

std::filesystem::path under(const std::filesystem::path& root, std::string_view relative) {
    if (relative.starts_with("./")) relative.remove_prefix(2);
    return root / std::filesystem::path(relative);
}

}

void locateExecutable(const char* argv0) {
    if (argv0 == nullptr || *argv0 == '\0') return;

    std::error_code error;
    const std::filesystem::path path = std::filesystem::weakly_canonical(argv0, error);
    if (!error) exeDir = path.parent_path();
}

const std::filesystem::path& executableDir() { return exeDir; }

const std::filesystem::path& engineRoot() {
    if (!engineResolved) {
        engine = resolveEngine();
        engineResolved = true;
    }
    return engine;
}

std::filesystem::path enginePath(std::string_view relative) { return under(engineRoot(), relative); }

void setProjectRoot(const std::filesystem::path& root) { project = std::filesystem::absolute(root); }

const std::filesystem::path& projectRoot() { return project; }

std::filesystem::path projectPath(std::string_view relative) { return under(project, relative); }

}
