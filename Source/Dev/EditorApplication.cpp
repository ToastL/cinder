#include "Dev/EditorApplication.hpp"

#include "Core/Engine.hpp"
#include "Core/GameLoop.hpp"
#include "Core/LaunchOptions.hpp"
#include "Core/ProjectConfig.hpp"
#include "Dev/EditorShell.hpp"
#include "Dev/GameBuild.hpp"
#include "Dev/ModuleHost.hpp"
#include "Dev/Packager.hpp"
#include "Platform/Assets.hpp"
#include "Platform/InputScript.hpp"
#include "Platform/Log.hpp"

#include <filesystem>
#include <optional>
#include <string>

using cinder::Core::Engine;
using cinder::Core::GameLoop;

namespace cinder::Dev {

void runEditor(const Core::ProjectConfig& config, const Core::LaunchOptions& options) {
    const std::filesystem::path scenePath = cinder::Platform::contentPath(config.startScene);
    Engine engine(config);
    Packager packager(cinder::Platform::projectRoot(), config.descriptor.targetPlatforms);
    ModuleHost modules(engine.scene(), cinder::Platform::projectRoot(), config.name, GameBuild::locate());
    EditorShell shell(engine, modules, packager, scenePath);
    PlaySession& session = shell.session();
    History& history = shell.history();
    Panels::Toolbar& toolbar = shell.toolbar();

    cinder::Platform::Input& input = engine.input();
    input.setRecording(true);
    std::optional<cinder::Platform::InputScript> steps;
    if (!options.inputScript.empty()) {
        steps = cinder::Platform::InputScript::load(options.inputScript);
        input.setIgnoreSystem(true);
    }

    if (!modules.available()) {
        cinder::Platform::logError("[build] game C++ is unavailable: cbt was not found next to the editor\n");
    } else if (modules.hasSources()) {
        modules.buildNow();
    }

    if (std::filesystem::exists(scenePath)) {
        engine.openScene(scenePath);
        cinder::Platform::logInfo("[editor] opened %s\n", scenePath.string().c_str());
    } else {
        cinder::Platform::logInfo("[editor] new scene, Save writes %s\n", scenePath.string().c_str());
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
