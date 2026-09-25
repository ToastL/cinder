#include "platform/Assets.hpp"

#include "platform/Log.hpp"

#include <cstdlib>
#include <stdexcept>
#include <string>
#include <system_error>

#ifndef CINDER_ENGINE_DEFAULT
#define CINDER_ENGINE_DEFAULT "engine"
#endif

#ifndef CINDER_SHADERS_DEFAULT
#define CINDER_SHADERS_DEFAULT "engine/shaders"
#endif

namespace cinder::platform {
namespace {

std::filesystem::path exeDir;
std::filesystem::path engine;
std::filesystem::path shaders;
std::filesystem::path project;
bool engineResolved = false;
bool shadersResolved = false;

std::filesystem::path resolveEngine() {
    if (const char* env = std::getenv("CINDER_ENGINE"); env != nullptr && *env != '\0') return env;

    if (!exeDir.empty() && std::filesystem::exists(exeDir / "engine")) return exeDir / "engine";

    const std::filesystem::path baked(CINDER_ENGINE_DEFAULT);
    if (std::filesystem::exists(baked)) return baked;

    return std::filesystem::path("engine");
}

std::filesystem::path resolveShaders() {
    if (const char* env = std::getenv("CINDER_SHADERS"); env != nullptr && *env != '\0') return env;

    if (!exeDir.empty() && std::filesystem::exists(exeDir / "engine" / "shaders")) {
        return exeDir / "engine" / "shaders";
    }

    const std::filesystem::path baked(CINDER_SHADERS_DEFAULT);
    if (std::filesystem::exists(baked)) return baked;

    return engineRoot() / "shaders";
}

std::filesystem::path under(const std::filesystem::path& root, std::string_view relative) {
    if (relative.starts_with("./")) relative.remove_prefix(2);
    return root / std::filesystem::path(relative);
}

bool matchesDisk(const std::filesystem::path& local) {
    std::error_code error;
    if (!std::filesystem::exists(project / local, error)) return true;

    std::filesystem::path at = project;
    for (const std::filesystem::path& part : local) {
        if (part.empty() || part == ".") continue;

        bool found = false;
        for (const auto& entry : std::filesystem::directory_iterator(at, error)) {
            if (entry.path().filename() == part) {
                found = true;
                break;
            }
        }
        if (!found) return false;
        at /= part;
    }
    return true;
}

std::filesystem::path inside(const char* folder, std::string_view relative) {
    const std::filesystem::path given(relative);
    const std::filesystem::path normal = given.lexically_normal();
    if (given.has_root_path() || (!normal.empty() && *normal.begin() == "..")) {
        throw std::runtime_error("\"" + std::string(relative) + "\" is outside " + folder + "/");
    }

    const std::filesystem::path local = std::filesystem::path(folder) / normal;
    if (!matchesDisk(local)) {
        logError("[assets] %s differs in case from the file on disk\n", local.string().c_str());
    }
    return project / local;
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

const std::filesystem::path& shaderRoot() {
    if (!shadersResolved) {
        shaders = resolveShaders();
        shadersResolved = true;
    }
    return shaders;
}

std::filesystem::path shaderPath(std::string_view relative) { return under(shaderRoot(), relative); }

void setProjectRoot(const std::filesystem::path& root) { project = std::filesystem::absolute(root); }

const std::filesystem::path& projectRoot() { return project; }

std::filesystem::path contentPath(std::string_view relative) { return inside("Content", relative); }

std::filesystem::path sourcePath(std::string_view relative) { return inside("Source", relative); }

}
