#include "core/GameModule.hpp"
#include "core/ProjectConfig.hpp"
#include "dev/Staging.hpp"
#include "dev/Toolchain.hpp"
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
        request.player = cinder::platform::executableDir() / "player";
        request.engineDir = cinder::platform::engineRoot();
        request.shaderDir = cinder::platform::shaderRoot();

        std::string error;
        request.toolchain = cinder::dev::Toolchain::load(cinder::dev::Toolchain::defaultPath(), error);

        cinder::dev::stage(request);
        return 0;
    } catch (const std::exception& e) {
        cinder::platform::logError("error: %s\n", e.what());
        return 1;
    }
}
