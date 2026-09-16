#include "core/Engine.hpp"
#include "core/GameLoop.hpp"
#include "core/ProjectConfig.hpp"
#include "platform/Assets.hpp"
#include "platform/Glfw.hpp"
#include "platform/Log.hpp"

#include <cstdio>
#include <cstring>
#include <exception>
#include <filesystem>
#include <string>

using cinder::core::Engine;
using cinder::core::GameLoop;
using cinder::core::ProjectConfig;
using cinder::platform::Glfw;

int main(int argc, char** argv) {
    std::setvbuf(stdout, nullptr, _IOLBF, 0);
    cinder::platform::locateExecutable(argv[0]);

    int frameLimit = 0;
    std::string capture;
    std::string scene;
    std::string project;

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--frames") == 0 && i + 1 < argc) {
            frameLimit = std::atoi(argv[++i]);
        } else if (std::strcmp(argv[i], "--capture") == 0 && i + 1 < argc) {
            capture = argv[++i];
        } else if (std::strcmp(argv[i], "--scene") == 0 && i + 1 < argc) {
            scene = argv[++i];
        } else if (argv[i][0] != '-' && project.empty()) {
            project = argv[i];
        }
    }

    try {
        const std::filesystem::path root = ProjectConfig::root(project.empty()
                ? cinder::platform::executableDir() / "project"
                : std::filesystem::path(project));
        cinder::platform::setProjectRoot(root);
        ProjectConfig config = ProjectConfig::load(root);
        if (!scene.empty()) config.startScene = scene;

        Glfw::acquire();
        {
            Engine engine(config);
            engine.openScene(cinder::platform::contentPath(config.startScene));
            GameLoop loop(config.fixedHz);

            int frames = 0;
            while (engine.running()) {
                loop.tick(engine);
                if (frameLimit > 0 && ++frames >= frameLimit) break;
            }

            if (!capture.empty()) engine.renderer().capture(capture);
        }
        Glfw::release();
        return 0;
    } catch (const std::exception& e) {
        cinder::platform::logError("[fatal] %s\n", e.what());
        return 1;
    }
}
