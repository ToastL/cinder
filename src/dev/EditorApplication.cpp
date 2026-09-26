#include "dev/EditorApplication.hpp"

#include "core/Engine.hpp"
#include "core/GameLoop.hpp"
#include "core/LaunchOptions.hpp"
#include "core/ProjectConfig.hpp"
#include "dev/History.hpp"
#include "dev/Packager.hpp"
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
#include "platform/InputScript.hpp"
#include "platform/Log.hpp"
#include "text/FontSet.hpp"
#include "ui/docking/TabManager.hpp"
#include "ui/core/ElementList.hpp"
#include "ui/framework/Application.hpp"
#include "ui/framework/WindowPlatform.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

using cinder::core::Engine;
using cinder::core::GameLoop;

namespace cinder::dev {

void runEditor(const core::ProjectConfig& config, const core::LaunchOptions& options) {
    const std::filesystem::path scenePath = cinder::platform::contentPath(config.startScene);
    Engine engine(config);
    PlaySession session(engine);
    Selection selection;
    History history(engine.scene());
    Packager packager(cinder::platform::projectRoot(), config.descriptor.targetPlatforms);

    const cinder::text::FontSet fonts = cinder::text::FontSet::engineDefault();
    cinder::ui::WindowPlatform platform(engine.window());
    cinder::ui::Application app(platform, fonts, cinder::dev::panels::editorTheme());
    cinder::dev::panels::Toolbar toolbar(session, history, selection, packager, scenePath, app);
    cinder::dev::panels::SceneView view(session, selection, history, engine, app);
    cinder::dev::panels::Explorer explorer(selection, history, engine.scene(), app);
    cinder::dev::panels::Properties properties(selection, history, engine.scene(), app);
    cinder::dev::panels::Console console(engine.script(), history, selection, app);
    cinder::ui::TabManager tabs;
    app.setRoot(cinder::dev::panels::editorLayout(toolbar, view, explorer, properties, console, tabs));

    cinder::platform::Input& input = engine.input();
    input.setRecording(true);
    std::optional<cinder::platform::InputScript> steps;
    if (!options.inputScript.empty()) {
        steps = cinder::platform::InputScript::load(options.inputScript);
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
    if (options.play) session.togglePlay();

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
        const bool last = options.frameLimit > 0 && frames + 1 >= options.frameLimit;
        if (last && !options.windowCapture.empty()) engine.renderer().requestWindowCapture(options.windowCapture);
        session.tick(loop);
        ++frames;
        if (last) break;
    }

    if (!options.capture.empty()) engine.renderer().capture(options.capture);
    engine.renderer().setUiPaint(nullptr);
    app.setRoot(nullptr);
}

}
