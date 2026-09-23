#include "core/Engine.hpp"
#include "core/GameLoop.hpp"
#include "core/ProjectConfig.hpp"
#include "dev/History.hpp"
#include "dev/PlaySession.hpp"
#include "dev/Selection.hpp"
#include "dev/panels/Console.hpp"
#include "dev/panels/EditorStyle.hpp"
#include "dev/panels/Explorer.hpp"
#include "dev/panels/Properties.hpp"
#include "dev/panels/Layout.hpp"
#include "dev/panels/SceneView.hpp"
#include "dev/panels/Toolbar.hpp"
#include "platform/Assets.hpp"
#include "platform/Glfw.hpp"
#include "platform/InputScript.hpp"
#include "platform/Log.hpp"
#include "text/FontSet.hpp"
#include "ui/docking/TabManager.hpp"
#include "ui/core/ElementList.hpp"
#include "ui/framework/Application.hpp"
#include "ui/framework/WindowPlatform.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

using cinder::core::Engine;
using cinder::core::GameLoop;
using cinder::core::ProjectConfig;
using cinder::dev::History;
using cinder::dev::PlaySession;
using cinder::dev::Selection;
using cinder::platform::Glfw;

int main(int argc, char** argv) {
    std::setvbuf(stdout, nullptr, _IOLBF, 0);
    cinder::platform::locateExecutable(argv[0]);

    int frameLimit = 0;
    bool play = false;
    std::string capture;
    std::string windowCapture;
    std::string script;
    std::string scene;
    std::string project;

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--frames") == 0 && i + 1 < argc) {
            frameLimit = std::atoi(argv[++i]);
        } else if (std::strcmp(argv[i], "--capture") == 0 && i + 1 < argc) {
            capture = argv[++i];
        } else if (std::strcmp(argv[i], "--capture-window") == 0 && i + 1 < argc) {
            windowCapture = argv[++i];
        } else if (std::strcmp(argv[i], "--input-script") == 0 && i + 1 < argc) {
            script = argv[++i];
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
                " [--capture png] [--capture-window png] [--input-script file]\n");
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
            Engine engine(config);
            PlaySession session(engine);
            Selection selection;
            History history(engine.scene());

            const cinder::text::FontSet fonts = cinder::text::FontSet::engineDefault();
            cinder::ui::WindowPlatform platform(engine.window());
            cinder::ui::Application app(platform, fonts, cinder::dev::panels::editorTheme());
            cinder::dev::panels::Toolbar toolbar(session, history, selection, scenePath, app);
            cinder::dev::panels::SceneView view(session, selection, history, engine, app);
            cinder::dev::panels::Explorer explorer(selection, history, engine.scene(), app);
            cinder::dev::panels::Properties properties(selection, history, engine.scene(), app);
            cinder::dev::panels::Console console(engine.script(), history, selection, app);
            cinder::ui::TabManager tabs;
            app.setRoot(cinder::dev::panels::editorLayout(toolbar, view, explorer, properties, console, tabs));

            cinder::platform::Input& input = engine.input();
            input.setRecording(true);
            std::optional<cinder::platform::InputScript> steps;
            if (!script.empty()) {
                steps = cinder::platform::InputScript::load(script);
                input.setIgnoreSystem(true);
            }

            engine.renderer().setUiPaint([&](cinder::ui::ElementList& list) {
                const std::vector<cinder::platform::InputEvent> events = input.takeEvents();
                app.processEvents(events);
                toolbar.update();
                view.update();
                explorer.update();
                properties.update();
                console.update();
                app.paint(list);
                view.afterPaint();
            });

            if (std::filesystem::exists(scenePath)) {
                engine.openScene(scenePath);
                cinder::platform::logInfo("[editor] opened %s\n", scenePath.string().c_str());
            } else {
                cinder::platform::logInfo("[editor] new scene, Save writes %s\n", scenePath.string().c_str());
            }
            history.reset();
            if (play) session.togglePlay();

            GameLoop loop(config.fixedHz);

            int frames = 0;
            while (!toolbar.closeConfirmed()) {
                if (engine.window().shouldClose()) {
                    engine.window().setShouldClose(false);
                    toolbar.requestClose();
                    if (toolbar.closeConfirmed()) break;
                }
                if (steps) {
                    steps->apply(input, frames);
                    if (std::optional<std::string> path = steps->capture(frames)) {
                        engine.renderer().requestWindowCapture(*path);
                    }
                    if (steps->closes(frames)) engine.window().setShouldClose(true);
                    if (steps->quits(frames)) break;
                }
                const bool last = frameLimit > 0 && frames + 1 >= frameLimit;
                if (last && !windowCapture.empty()) engine.renderer().requestWindowCapture(windowCapture);
                session.tick(loop);
                ++frames;
                if (last) break;
            }

            if (!capture.empty()) engine.renderer().capture(capture);
            engine.renderer().setUiPaint(nullptr);
            app.setRoot(nullptr);
        }
        Glfw::release();
        return 0;
    } catch (const std::exception& e) {
        cinder::platform::logError("[fatal] %s\n", e.what());
        return 1;
    }
}
