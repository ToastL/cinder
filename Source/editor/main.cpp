#include "core/LaunchOptions.hpp"
#include "core/ProjectConfig.hpp"
#include "dev/EditorApplication.hpp"
#include "platform/Assets.hpp"
#include "platform/Glfw.hpp"
#include "platform/Log.hpp"

#include <cstdio>
#include <exception>
#include <filesystem>

using cinder::core::ProjectConfig;

int main(int argc, char** argv) {
    std::setvbuf(stdout, nullptr, _IOLBF, 0);
    cinder::platform::locateExecutable(argv[0]);

    const auto options = cinder::core::LaunchOptions::parse(argc, argv, cinder::core::LaunchTarget::Editor);

    if (options.project.empty()) {
        cinder::platform::logError(
                "usage: editor <project folder or .cinder file> [--scene path] [--play] [--frames n]"
                " [--capture png] [--capture-window png] [--input-script file]\n");
        return 2;
    }

    try {
        const std::filesystem::path root = ProjectConfig::root(options.project);
        cinder::platform::setProjectRoot(root);
        ProjectConfig config = ProjectConfig::load(root);
        if (!options.scene.empty()) config.startScene = options.scene;
        const cinder::platform::GlfwSession glfw;
        cinder::dev::runEditor(config, options);
        return 0;
    } catch (const std::exception& e) {
        cinder::platform::logError("[fatal] %s\n", e.what());
        return 1;
    }
}
