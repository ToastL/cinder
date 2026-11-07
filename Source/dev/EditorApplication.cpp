#include "dev/EditorApplication.hpp"

#include "core/Engine.hpp"
#include "core/GameLoop.hpp"
#include "core/LaunchOptions.hpp"
#include "core/ProjectConfig.hpp"
#include "dev/EditorShell.hpp"
#include "dev/GameBuild.hpp"
#include "dev/ModuleHost.hpp"
#include "dev/Packager.hpp"
#include "platform/Assets.hpp"
#include "platform/InputScript.hpp"
#include "platform/Log.hpp"

#include <filesystem>
#include <optional>
#include <string>

using cinder::core::Engine;
using cinder::core::GameLoop;

namespace cinder::dev {

void runEditor(const core::ProjectConfig& config, const core::LaunchOptions& options) {
    const std::filesystem::path scenePath = cinder::platform::contentPath(config.startScene);
    Engine engine(config);
    Packager packager(cinder::platform::projectRoot(), config.descriptor.targetPlatforms);
    ModuleHost modules(engine.scene(), cinder::platform::projectRoot(), config.name, GameBuild::locate());
    EditorShell shell(engine, modules, packager, scenePath);
    PlaySession& session = shell.session();
    History& history = shell.history();
    panels::Toolbar& toolbar = shell.toolbar();

    cinder::platform::Input& input = engine.input();
    input.setRecording(true);
    std::optional<cinder::platform::InputScript> steps;
    if (!options.inputScript.empty()) {
        steps = cinder::platform::InputScript::load(options.inputScript);
        input.setIgnoreSystem(true);
    }

    if (!modules.available()) {
        cinder::platform::logError("[build] game C++ is unavailable: cbt was not found next to the editor\n");
    } else if (modules.hasSources()) {
        modules.buildNow();
    }

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
        modules.update();
        shell.applyPendingReload();
        session.tick(loop);
        ++frames;
        if (last) break;
    }

    if (!options.capture.empty()) engine.renderer().capture(options.capture);
}

}
