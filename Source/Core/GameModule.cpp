#include "Core/GameModule.hpp"

#include "Core/Module.hpp"
#include "Platform/Log.hpp"
#include "Scene/ClassRegistry.hpp"

#include <dlfcn.h>

namespace cinder::Core {
namespace {

using AbiFn = const char* (*)();

}

std::string_view GameModule::abi() { return buildId(); }

std::string_view GameModule::extension() {
#if defined(__APPLE__)
    return ".dylib";
#else
    return ".so";
#endif
}

std::string_view GameModule::platform() {
#if defined(__APPLE__)
    return "macOS";
#else
    return "Linux";
#endif
}

std::optional<std::filesystem::path> GameModule::locate(const std::filesystem::path& project, std::string_view name) {
    const std::string file = std::string(name) + std::string(extension());
    for (const std::filesystem::path& candidate :
         {project / "Binaries" / std::string(platform()) / file, project / "Saved" / "Binaries" / file}) {
        std::error_code error;
        if (std::filesystem::is_regular_file(candidate, error)) return candidate;
    }
    return std::nullopt;
}

std::unique_ptr<GameModule> GameModule::open(const std::filesystem::path& library,
                                             cinder::Scene::ClassRegistry& types, std::string& error) {
    void* handle = dlopen(library.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (handle == nullptr) {
        const char* reason = dlerror();
        error = reason != nullptr ? reason : "dlopen failed";
        return nullptr;
    }

    const auto abiOf = reinterpret_cast<AbiFn>(dlsym(handle, "cinder_module_abi"));
    const auto enlist = reinterpret_cast<RegisterFn>(dlsym(handle, "cinder_module_register"));
    if (abiOf == nullptr || enlist == nullptr) {
        error = library.filename().string() + " is not a cinder game module";
        dlclose(handle);
        return nullptr;
    }
    if (std::string_view(abiOf()) != abi()) {
        error = library.filename().string() + " was built for " + abiOf() + ", this engine is "
                + std::string(abi());
        dlclose(handle);
        return nullptr;
    }
    return std::unique_ptr<GameModule>(new GameModule(handle, enlist, library, types));
}

GameModule::GameModule(void* handle, RegisterFn enlist, std::filesystem::path path, cinder::Scene::ClassRegistry& types)
    : handle_(handle), register_(enlist), path_(std::move(path)), types_(types) {}

GameModule::~GameModule() {
    for (const std::string& name : classes_) types_.remove(name);
    dlclose(handle_);
}

void GameModule::enlist() {
    if (!classes_.empty()) return;
    cinder::Core::Registrar registrar(types_);
    register_(registrar);
    classes_ = registrar.added();
    for (const std::string& name : registrar.refused()) {
        cinder::Platform::logError("[module] %s: class %s is already registered, skipped\n",
                                   path_.filename().string().c_str(), name.c_str());
    }
}

}
