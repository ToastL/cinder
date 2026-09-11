#include "core/Engine.hpp"
#include "core/GameLoop.hpp"
#include "core/ProjectConfig.hpp"
#include "dev/Console.hpp"
#include "dev/Dockspace.hpp"
#include "dev/ImGuiLayer.hpp"
#include "dev/PlaySession.hpp"
#include "dev/Toolbar.hpp"
#include "dev/Viewport.hpp"
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
using cinder::dev::Console;
using cinder::dev::PlaySession;
using cinder::dev::Toolbar;
using cinder::dev::Viewport;
using cinder::platform::Glfw;

int main(int argc, char** argv) {
    std::setvbuf(stdout, nullptr, _IOLBF, 0);
    cinder::platform::locateExecutable(argv[0]);

    int frameLimit = 0;
    bool play = false;
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
        } else if (std::strcmp(argv[i], "--play") == 0) {
            play = true;
        } else if (argv[i][0] != '-' && project.empty()) {
            project = argv[i];
        }
    }

    if (project.empty()) {
        cinder::platform::logError(
                "usage: editor <project> [--scene path] [--play] [--frames n] [--capture png]\n");
        return 2;
    }

    try {
        cinder::platform::setProjectRoot(project);
        ProjectConfig config = ProjectConfig::load(cinder::platform::projectPath("project.lua"));
        if (!scene.empty()) config.scene = scene;
        const std::filesystem::path scenePath = cinder::platform::projectPath(config.scene);

        Glfw::acquire();
        {
            Engine engine(config, cinder::dev::overlayFactory());
            PlaySession session(engine);
            Console console(engine.script());
            Toolbar toolbar(session, engine, scenePath);
            Viewport viewport(session, engine);
            engine.renderer().setOverlayDraw([&toolbar, &viewport, &console] {
                toolbar.draw();
                cinder::dev::drawDockspace();
                viewport.draw();
                console.draw();
            });

            if (std::filesystem::exists(scenePath)) {
                engine.openScene(scenePath);
                cinder::platform::logInfo("[editor] opened %s\n", scenePath.string().c_str());
            } else {
                cinder::platform::logInfo("[editor] new scene, Save writes %s\n",
                                          scenePath.string().c_str());
            }
            if (play) session.togglePlay();

            GameLoop loop(config.fixedHz);

            int frames = 0;
            while (!engine.window().shouldClose()) {
                session.tick(loop);
                if (frameLimit > 0 && ++frames >= frameLimit) break;
            }

            if (!capture.empty()) engine.renderer().capture(capture);
            engine.renderer().setOverlayDraw(nullptr);
        }
        Glfw::release();
        return 0;
    } catch (const std::exception& e) {
        cinder::platform::logError("[fatal] %s\n", e.what());
        return 1;
    }
}
