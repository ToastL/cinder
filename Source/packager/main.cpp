#include "core/GameModule.hpp"
#include "core/ProjectConfig.hpp"
#include "dev/Staging.hpp"
#include "dev/GameBuild.hpp"
#include "dev/Process.hpp"
#include "platform/Assets.hpp"
#include "platform/Log.hpp"

#include <cstdio>
#include <exception>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

using cinder::core::ProjectConfig;

namespace {

int usage() {
    cinder::platform::logError(
            "usage: packager <project folder or .cinder file> [--platform macOS|Linux] "
            "[--config development|shipping] [--out dir]\n");
    return 2;
}

}

int main(int argc, char** argv) {
    std::setvbuf(stdout, nullptr, _IOLBF, 0);
    cinder::platform::locateExecutable(argv[0]);

    std::string project;
    cinder::dev::Platform platform = cinder::dev::hostPlatform();
    cinder::dev::Configuration configuration = cinder::dev::Configuration::Development;
    std::filesystem::path out;

    for (int i = 1; i < argc; ++i) {
        const std::string_view arg = argv[i];
        const bool hasValue = i + 1 < argc;
        if (arg == "--platform" && hasValue) {
            const std::optional<cinder::dev::Platform> named = cinder::dev::platformNamed(argv[++i]);
            if (!named) return usage();
            platform = *named;
        } else if (arg == "--config" && hasValue) {
            const std::optional<cinder::dev::Configuration> named = cinder::dev::configurationNamed(argv[++i]);
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
        if (!cinder::dev::platformSupported(platform)) {
            throw std::runtime_error(std::string(cinder::dev::platformName(platform)) + " cannot be packaged on this machine");
        }
        const std::filesystem::path root = ProjectConfig::root(project);
        const ProjectConfig config = ProjectConfig::load(root);

        cinder::dev::StageRequest request;
        request.project = root;
        request.platform = platform;
        request.configuration = configuration;
        request.out = out.empty() ? root / "Saved" / "Builds" / std::string(cinder::dev::platformName(platform)) / config.name
                                  : std::filesystem::absolute(out);
        request.engineDir = cinder::platform::engineRoot();
        request.build = cinder::dev::GameBuild::locate();
        if (request.build) {
            request.build->config = std::string(cinder::dev::configurationName(configuration));
            std::string error;
            std::unique_ptr<cinder::dev::Process> process = cinder::dev::Process::start(request.build->build("player"), error);
            if (process == nullptr) throw std::runtime_error(error);
            std::string line;
            while (process->readLine(line)) cinder::platform::logInfo("%s\n", line.c_str());
            if (process->wait() != 0) throw std::runtime_error("cbt could not build the player");
            request.player = request.build->binaries() / "player";
            request.shaderDir = request.build->binaries() / "Shaders";
        } else {
            request.player = cinder::platform::executableDir() / "player";
            request.shaderDir = cinder::platform::shaderRoot();
        }

        cinder::dev::stage(request);
        return 0;
    } catch (const std::exception& e) {
        cinder::platform::logError("error: %s\n", e.what());
        return 1;
    }
}
