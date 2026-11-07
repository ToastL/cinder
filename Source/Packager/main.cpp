#include "Core/ProjectConfig.hpp"
#include "Dev/GameBuild.hpp"
#include "Dev/Process.hpp"
#include "Dev/Staging.hpp"
#include "Platform/Assets.hpp"
#include "Platform/Log.hpp"

#include <cstdio>
#include <exception>
#include <filesystem>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

using cinder::Core::ProjectConfig;

namespace {

int usage() {
    cinder::Platform::logError(
            "usage: packager <project folder or .cinder file> [--platform macOS|Linux|Windows|iOS|Android] "
            "[--config development|shipping] [--out dir]\n");
    return 2;
}

}

int main(int argc, char** argv) {
    std::setvbuf(stdout, nullptr, _IOLBF, 0);
    cinder::Platform::locateExecutable(argv[0]);

    std::string project;
    cinder::Dev::Platform platform = cinder::Dev::hostPlatform();
    cinder::Dev::Configuration configuration = cinder::Dev::Configuration::Development;
    std::filesystem::path out;

    for (int i = 1; i < argc; ++i) {
        const std::string_view arg = argv[i];
        const bool hasValue = i + 1 < argc;
        if (arg == "--platform" && hasValue) {
            const std::optional<cinder::Dev::Platform> named = cinder::Dev::platformNamed(argv[++i]);
            if (!named) return usage();
            platform = *named;
        } else if (arg == "--config" && hasValue) {
            const std::optional<cinder::Dev::Configuration> named = cinder::Dev::configurationNamed(argv[++i]);
            if (!named) return usage();
            configuration = *named;
        } else if (arg == "--out" && hasValue) {
            out = argv[++i];
        } else if (!arg.starts_with("--") && project.empty()) {
            project = std::string(arg);
        } else {
            return usage();
        }
    }
    if (project.empty()) return usage();

    try {
        if (!cinder::Dev::platformSupported(platform)) {
            throw std::runtime_error(std::string(cinder::Dev::platformName(platform)) + " cannot be packaged on this machine");
        }
        const std::filesystem::path root = ProjectConfig::root(project);
        const ProjectConfig config = ProjectConfig::load(root);

        cinder::Dev::StageRequest request;
        request.project = root;
        request.platform = platform;
        request.configuration = configuration;
        request.out = out.empty() ? root / "Saved" / "Builds" / std::string(cinder::Dev::platformName(platform)) / config.name
                                  : std::filesystem::absolute(out);
        request.engineDir = cinder::Platform::engineRoot();
        request.build = cinder::Dev::GameBuild::locate();
        if (request.build) {
            request.build->config = std::string(cinder::Dev::configurationName(configuration));
            std::string error;
            std::unique_ptr<cinder::Dev::Process> process = cinder::Dev::Process::start(request.build->build("player"), error);
            if (process == nullptr) throw std::runtime_error(error);
            std::string line;
            while (process->readLine(line)) cinder::Platform::logInfo("%s\n", line.c_str());
            if (process->wait() != 0) throw std::runtime_error("cbt could not build the player");
            request.player = request.build->binaries() / "player";
            request.shaderDir = request.build->binaries() / "Shaders";
        } else {
            request.player = cinder::Platform::executableDir() / "player";
            request.shaderDir = cinder::Platform::shaderRoot();
        }

        cinder::Dev::stage(request);
        return 0;
    } catch (const std::exception& e) {
        cinder::Platform::logError("error: %s\n", e.what());
        return 1;
    }
}
