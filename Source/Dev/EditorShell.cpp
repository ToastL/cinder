#include "Dev/EditorShell.hpp"

#include "Core/Engine.hpp"
#include "Dev/ModuleHost.hpp"
#include "Dev/Panels/EditorStyle.hpp"
#include "Dev/Panels/Layout.hpp"
#include "UI/Core/ElementList.hpp"

#include <utility>
#include <vector>

namespace cinder::Dev {

EditorShell::EditorShell(cinder::Core::Engine& engine, ModuleHost& modules, Packager& packager,
                         std::filesystem::path scene)
    : engine_(engine), modules_(modules), session_(engine), history_(engine.scene()),
      document_(engine.scene(), history_, selection_, std::move(scene)),
      fonts_(cinder::Text::FontSet::engineDefault()), platform_(engine.window()),
      app_(platform_, fonts_, Panels::editorTheme()),
      toolbar_(session_, history_, selection_, packager, modules, document_, app_),
      view_(session_, selection_, history_, engine, app_), outliner_(selection_, history_, engine.scene(), app_),
      details_(selection_, history_, engine.scene(), app_), console_(app_) {
    document_.setOnSwitched([this] {
        view_.sceneChanged();
        outliner_.sceneChanged();
        details_.rebuild();
    });
    PrefabActions prefabs;
    prefabs.save = [this](int actor) { toolbar_.requestSavePrefab(actor); };
    prefabs.apply = [this](int actor) { document_.applyPrefab(actor); };
    prefabs.revert = [this](int actor) { document_.revertPrefab(actor); };
    prefabs.unpack = [this](int actor) { document_.unpackPrefab(actor); };
    prefabs.enabled = [this] { return session_.state() == PlaySession::State::Edit && !toolbar_.prompting(); };
    outliner_.setPrefabActions(prefabs);
    details_.setPrefabActions(std::move(prefabs));
    app_.setRoot(Panels::editorLayout(toolbar_, view_, outliner_, details_, console_, tabs_));
    engine.renderer().setUiPaint([this](cinder::UI::ElementList& list) {
        const std::vector<cinder::Platform::InputEvent> events = engine_.input().takeEvents();
        app_.processEvents(events);
        toolbar_.update();
        view_.update();
        outliner_.update();
        details_.update();
        console_.update();
        app_.paint(list);
        view_.afterPaint();
    });
}

EditorShell::~EditorShell() {
    document_.setOnSwitched(nullptr);
    engine_.renderer().setUiPaint(nullptr);
    app_.setRoot(nullptr);
}

bool EditorShell::applyPendingReload() {
    if (!modules_.reloadPending() || session_.state() != PlaySession::State::Edit || app_.isInteracting()
        || app_.hasPopups() || !modules_.reload()) {
        return false;
    }
    details_.rebuild();
    return true;
}

bool EditorShell::applyPendingScene() {
    if (!document_.pending() || session_.state() != PlaySession::State::Edit || app_.isInteracting()
        || app_.hasPopups()) {
        return false;
    }
    return document_.applyPending();
}

}
