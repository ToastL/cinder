#include "dev/Staging.hpp"

#include "core/GameModule.hpp"
#include "core/ProjectConfig.hpp"
#include "dev/ModuleBuilder.hpp"
#include "dev/Process.hpp"
#include "platform/Log.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <map>
#include <stdexcept>
#include <vector>

namespace cinder::dev {
namespace {

constexpr std::array<Platform, 5> PLATFORMS{Platform::MacOS, Platform::Linux, Platform::Windows, Platform::IOS,
                                            Platform::Android};

std::vector<std::string> shaderPatterns(const std::vector<std::string>& targets) {
    static const std::map<std::string, std::vector<std::string>, std::less<>> FORMATS{
            {"macOS", {".spv", ".metallib"}},
            {"iOS", {".metallib"}},
            {"Linux", {".spv"}},
            {"Android", {".spv"}},
            {"Windows", {".dxil"}},
    };
    const std::vector<std::string> all{"macOS", "Linux", "Windows"};
    std::vector<std::string> out;
    for (const std::string& target : targets.empty() ? all : targets) {
        auto found = FORMATS.find(target);
        if (found == FORMATS.end()) continue;
        for (const std::string& pattern : found->second) {
            if (std::find(out.begin(), out.end(), pattern) == out.end()) out.push_back(pattern);
        }
    }
    return out;
}

std::string expand(std::string step, const StageRequest& request) {
    const std::pair<std::string, std::string> variables[] = {
            {"$(ProjectDir)", request.project.string()},
            {"$(EngineDir)", request.engineDir.string()},
            {"$(StageDir)", request.out.string()},
    };
    for (const auto& [name, value] : variables) {
        for (std::size_t at = step.find(name); at != std::string::npos; at = step.find(name, at + value.size())) {
            step.replace(at, name.size(), value);
        }
    }
    return step;
}

void runSteps(const char* kind, const core::ProjectDescriptor::BuildSteps& steps, const StageRequest& request) {
    auto found = steps.find(std::string(platformName(request.platform)));
    if (found == steps.end()) return;
    for (const std::string& raw : found->second) {
        const std::string step = expand(raw, request);
        cinder::platform::logInfo("%s: %s\n", kind, step.c_str());
        const int status = std::system(("cd " + quote(request.project) + " && " + step).c_str());
        if (status != 0) throw std::runtime_error(std::string(kind) + " failed: " + step);
    }
}

void copyTree(const std::filesystem::path& from, const std::filesystem::path& to) {
    std::filesystem::create_directories(to);
    std::filesystem::copy(from, to, std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
}

void copyMatching(const std::filesystem::path& from, const std::filesystem::path& to,
                  const std::vector<std::string>& suffixes, bool warnMissing) {
    std::filesystem::create_directories(to);
    for (const std::string& suffix : suffixes) {
        bool found = false;
        std::error_code error;
        for (const auto& entry : std::filesystem::directory_iterator(from, error)) {
            const std::string name = entry.path().filename().string();
            if (!entry.is_regular_file() || !name.ends_with(suffix)) continue;
            std::filesystem::copy_file(entry.path(), to / name, std::filesystem::copy_options::overwrite_existing);
            found = true;
        }
        if (!found && warnMissing) {
            cinder::platform::logError("warning: no *%s in %s; targetPlatforms asks for it, but the build did not produce it\n",
                                       suffix.c_str(), from.string().c_str());
        }
    }
}

void strip(const std::filesystem::path& library) {
    std::string error;
#if defined(__APPLE__)
    std::unique_ptr<Process> process = Process::start({"strip", "-x", library.string()}, error);
#else
    std::unique_ptr<Process> process = Process::start({"strip", "--strip-unneeded", library.string()}, error);
#endif
    if (process == nullptr) throw std::runtime_error(error);
    std::string line;
    while (process->readLine(line)) cinder::platform::logInfo("%s\n", line.c_str());
    if (process->wait() != 0) throw std::runtime_error("strip failed on " + library.string());
}

std::filesystem::path buildModule(const StageRequest& request, const std::string& name) {
    const std::string config(configurationName(request.configuration));
    if (!request.toolchain) throw std::runtime_error("the project has C++ in Source/ but there is no toolchain manifest");
    Toolchain toolchain = *request.toolchain;
    toolchain.flags.push_back("-O2");

    const std::filesystem::path library = request.project / "Saved" / "Binaries" / ("Package-" + config)
            / (name + std::string(cinder::core::GameModule::extension()));
    ModuleBuilder builder(request.project, name, std::move(toolchain), library, "Package-" + config);
    const ModuleBuilder::Result result =
            builder.build([](const std::string& line) { cinder::platform::logInfo("%s\n", line.c_str()); });
    if (!result.ok) {
        throw std::runtime_error("the project's C++ did not build (" + std::to_string(result.errors)
                                 + (result.errors == 1 ? " error)" : " errors)"));
    }
    return library;
}

}

std::span<const Platform> platforms() { return PLATFORMS; }

std::string_view platformName(Platform platform) {
    switch (platform) {
        case Platform::MacOS: return "macOS";
        case Platform::Linux: return "Linux";
        case Platform::Windows: return "Windows";
        case Platform::IOS: return "iOS";
        case Platform::Android: return "Android";
    }
    return "";
}

std::optional<Platform> platformNamed(std::string_view name) {
    for (const Platform platform : PLATFORMS) {
        if (platformName(platform) == name) return platform;
    }
    return std::nullopt;
}

Platform hostPlatform() {
#if defined(__APPLE__)
    return Platform::MacOS;
#elif defined(_WIN32)
    return Platform::Windows;
#else
    return Platform::Linux;
#endif
}

bool platformSupported(Platform platform) {
    return platform == hostPlatform() && (platform == Platform::MacOS || platform == Platform::Linux);
}

std::string_view configurationName(Configuration configuration) {
    return configuration == Configuration::Shipping ? "Shipping" : "Development";
}

std::optional<Configuration> configurationNamed(std::string_view name) {
    std::string lower(name);
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return static_cast<char>(c | 0x20); });
    if (lower == "development") return Configuration::Development;
    if (lower == "shipping") return Configuration::Shipping;
    return std::nullopt;
}

std::string quote(const std::filesystem::path& path) {
    std::string out = "'";
    for (const char c : path.string()) {
        if (c == '\'') {
            out += "'\\''";
        } else {
            out += c;
        }
    }
    return out + "'";
}

void stage(const StageRequest& request) {
    const core::ProjectConfig config = core::ProjectConfig::load(request.project);
    const std::string platform(platformName(request.platform));
    const std::vector<std::string>& targets = config.descriptor.targetPlatforms;
    if (!targets.empty() && std::find(targets.begin(), targets.end(), platform) == targets.end()) {
        throw std::runtime_error(config.name + ".cinder does not list " + platform + " in targetPlatforms");
    }

    runSteps("preBuildSteps", config.descriptor.preBuildSteps, request);

    std::optional<std::filesystem::path> library;
    if (std::filesystem::is_directory(request.project / "Source")) {
        bool hasCode = false;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(request.project / "Source")) {
            if (entry.path().extension() == ".cpp") hasCode = true;
        }
        if (hasCode) library = buildModule(request, config.name);
    }

    std::filesystem::remove_all(request.out);
    std::filesystem::create_directories(request.out / "project");

    const std::filesystem::path player = request.out / request.player.filename();
    std::filesystem::copy_file(request.player, player);
    std::filesystem::permissions(player, std::filesystem::status(request.player).permissions());

    copyMatching(request.shaderDir, request.out / "engine" / "shaders", shaderPatterns(targets), true);
    copyMatching(request.engineDir / "fonts", request.out / "engine" / "fonts", {".ttf", ".txt"}, false);

    std::filesystem::copy_file(request.project / (config.name + ".cinder"), request.out / "project" / (config.name + ".cinder"));
    for (const char* folder : {"Config", "Content"}) {
        if (std::filesystem::is_directory(request.project / folder)) {
            copyTree(request.project / folder, request.out / "project" / folder);
        }
    }

    if (library) {
        const std::filesystem::path binaries = request.out / "project" / "Binaries" / platform;
        std::filesystem::create_directories(binaries);
        const std::filesystem::path shipped = binaries / library->filename();
        std::filesystem::copy_file(*library, shipped);
        if (request.configuration == Configuration::Shipping) {
            const std::filesystem::path symbols = request.project / "Saved" / "Symbols" / platform;
            std::filesystem::create_directories(symbols);
            std::filesystem::copy_file(*library, symbols / library->filename(), std::filesystem::copy_options::overwrite_existing);
            strip(shipped);
        }
    }

    runSteps("postBuildSteps", config.descriptor.postBuildSteps, request);
    cinder::platform::logInfo("packaged %s into %s\n", request.project.string().c_str(), request.out.string().c_str());
}

}
