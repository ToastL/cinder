#include "Core/LaunchOptions.hpp"
#include "Core/ProjectConfig.hpp"
#include "Dev/EditorApplication.hpp"
#include "Platform/Assets.hpp"
#include "Platform/Glfw.hpp"
#include "Platform/Log.hpp"

#include <cstdio>
#include <exception>
#include <filesystem>

using cinder::Core::ProjectConfig;

int main(int argc, char** argv) {
    std::setvbuf(stdout, nullptr, _IOLBF, 0);
    cinder::Platform::locateExecutable(argv[0]);

    const auto options = cinder::Core::LaunchOptions::parse(argc, argv, cinder::Core::LaunchTarget::Editor);

    if (options.project.empty()) {
        cinder::Platform::logError(
                "usage: editor <project folder or .cinder file> [--scene path] [--play] [--frames n]"
                " [--capture png] [--capture-window png] [--input-script file]\n");
        return 2;
    }

    try {
        const std::filesystem::path root = ProjectConfig::root(options.project);
        cinder::Platform::setProjectRoot(root);
        ProjectConfig config = ProjectConfig::load(root);
        if (!options.scene.empty()) config.startScene = options.scene;
        const cinder::Platform::GlfwSession glfw;
        cinder::Dev::runEditor(config, options);
        return 0;
    } catch (const std::exception& e) {
        cinder::Platform::logError("[fatal] %s\n", e.what());
        return 1;
    }
}
