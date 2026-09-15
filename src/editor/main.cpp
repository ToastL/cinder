#include "core/Engine.hpp"
#include "core/GameLoop.hpp"
#include "core/ProjectConfig.hpp"
#include "dev/Console.hpp"
#include "dev/Dockspace.hpp"
#include "dev/Explorer.hpp"
#include "dev/History.hpp"
#include "dev/ImGuiLayer.hpp"
#include "dev/Properties.hpp"
#include "dev/PlaySession.hpp"
#include "dev/Selection.hpp"
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
using cinder::dev::Explorer;
using cinder::dev::History;
using cinder::dev::Properties;
using cinder::dev::PlaySession;
using cinder::dev::Selection;
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
                "usage: editor <project folder or .cinder file> [--scene path] [--play] [--frames n]"
                " [--capture png]\n");
        return 2;
    }

    try {
        const std::filesystem::path root = ProjectConfig::root(project);
        cinder::platform::setProjectRoot(root);
        ProjectConfig config = ProjectConfig::load(root);
        if (!scene.empty()) config.startScene = scene;
        const std::filesystem::path scenePath = cinder::platform::contentPath(config.startScene);

        Glfw::acquire();
        {
            Engine engine(config, cinder::dev::overlayFactory());
            PlaySession session(engine);
            Selection selection;
            History history(engine.scene());
            Console console(engine.script(), history, selection);
            Toolbar toolbar(session, history, selection, scenePath);
            Viewport viewport(session, selection, engine);
            Explorer explorer(selection, history, engine.scene());
            Properties properties(selection, history, engine.scene());
            engine.renderer().setOverlayDraw(
                    [&toolbar, &viewport, &explorer, &properties, &console] {
                        toolbar.draw();
                        cinder::dev::drawDockspace();
                        viewport.draw();
                        explorer.draw();
                        properties.draw();
                        console.draw();
                    });

            if (std::filesystem::exists(scenePath)) {
                engine.openScene(scenePath);
                cinder::platform::logInfo("[editor] opened %s\n", scenePath.string().c_str());
            } else {
                cinder::platform::logInfo("[editor] new scene, Save writes %s\n",
                                          scenePath.string().c_str());
            }
            history.reset();
            if (play) session.togglePlay();

            GameLoop loop(config.fixedHz);

            void cinderProbe(GLFWwindow*, int);
            int frames = 0;
            while (!toolbar.closeConfirmed()) {
                if (engine.window().shouldClose()) {
                    engine.window().setShouldClose(false);
                    toolbar.requestClose();
                    if (toolbar.closeConfirmed()) break;
                }
                cinderProbe(engine.window().handle(), frames);
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
