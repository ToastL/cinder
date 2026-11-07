#include "core/Engine.hpp"
#include "core/GameLoop.hpp"
#include "core/GameModule.hpp"
#include "core/LaunchOptions.hpp"
#include "core/ProjectConfig.hpp"
#include "platform/Assets.hpp"
#include "platform/Glfw.hpp"
#include "platform/Log.hpp"

#include <cstdio>
#include <exception>
#include <filesystem>
#include <memory>
#include <string>

using cinder::core::Engine;
using cinder::core::GameLoop;
using cinder::core::ProjectConfig;

namespace {

struct SceneClearer {
    Engine& engine;
    ~SceneClearer() { engine.scene().clear(); }
};

}

int main(int argc, char** argv) {
    std::setvbuf(stdout, nullptr, _IOLBF, 0);
    cinder::platform::locateExecutable(argv[0]);

    const auto options = cinder::core::LaunchOptions::parse(argc, argv, cinder::core::LaunchTarget::Player);

    try {
        const std::filesystem::path root = ProjectConfig::root(options.project.empty()
                ? cinder::platform::executableDir() / "Project"
                : std::filesystem::path(options.project));
        cinder::platform::setProjectRoot(root);
        ProjectConfig config = ProjectConfig::load(root);
        if (!options.scene.empty()) config.startScene = options.scene;

        const cinder::platform::GlfwSession glfw;
        {
            Engine engine(config);
            std::unique_ptr<cinder::core::GameModule> module;
            const SceneClearer clearer{engine};
            if (const auto library = cinder::core::GameModule::locate(root, config.name)) {
                std::string error;
                module = cinder::core::GameModule::open(*library, engine.types(), error);
                if (module == nullptr) cinder::platform::logError("[module] %s\n", error.c_str());
                else module->enlist();
            }
            engine.openScene(cinder::platform::contentPath(config.startScene));
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
        cinder::platform::logError("[fatal] %s\n", e.what());
        return 1;
    }
}
