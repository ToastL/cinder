#include "Core/Engine.hpp"
#include "Core/GameLoop.hpp"
#include "Core/GameModule.hpp"
#include "Core/LaunchOptions.hpp"
#include "Core/ProjectConfig.hpp"
#include "Platform/Assets.hpp"
#include "Platform/Glfw.hpp"
#include "Platform/Log.hpp"

#include <cstdio>
#include <exception>
#include <filesystem>
#include <memory>
#include <string>

using cinder::Core::Engine;
using cinder::Core::GameLoop;
using cinder::Core::ProjectConfig;

namespace {

struct SceneClearer {
    Engine& engine;
    ~SceneClearer() { engine.scene().clear(); }
};

}

int main(int argc, char** argv) {
    std::setvbuf(stdout, nullptr, _IOLBF, 0);
    cinder::Platform::locateExecutable(argv[0]);

    const auto options = cinder::Core::LaunchOptions::parse(argc, argv, cinder::Core::LaunchTarget::Player);

    try {
        const std::filesystem::path root = ProjectConfig::root(options.project.empty()
                ? cinder::Platform::executableDir() / "Project"
                : std::filesystem::path(options.project));
        cinder::Platform::setProjectRoot(root);
        ProjectConfig config = ProjectConfig::load(root);
        if (!options.scene.empty()) config.startScene = options.scene;

        const cinder::Platform::GlfwSession glfw;
        {
            Engine engine(config);
            std::unique_ptr<cinder::Core::GameModule> module;
            const SceneClearer clearer{engine};
            if (const auto library = cinder::Core::GameModule::locate(root, config.name)) {
                std::string error;
                module = cinder::Core::GameModule::open(*library, engine.types(), error);
                if (module == nullptr) cinder::Platform::logError("[module] %s\n", error.c_str());
                else module->enlist();
            }
            engine.openScene(cinder::Platform::contentPath(config.startScene));
            GameLoop loop(config.fixedHz);

            int frames = 0;
            while (engine.running()) {
                loop.tick(engine);
                if (options.frameLimit > 0 && ++frames >= options.frameLimit) break;
            }

            if (!options.capture.empty()) engine.renderer().capture(options.capture);
        }
        return 0;
    } catch (const std::exception& e) {
        cinder::Platform::logError("[fatal] %s\n", e.what());
        return 1;
    }
}
