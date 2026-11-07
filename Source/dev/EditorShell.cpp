#include "dev/EditorShell.hpp"

#include "core/Engine.hpp"
#include "dev/ModuleHost.hpp"
#include "dev/panels/EditorStyle.hpp"
#include "dev/panels/Layout.hpp"
#include "ui/core/ElementList.hpp"

#include <utility>
#include <vector>

namespace cinder::dev {

EditorShell::EditorShell(cinder::core::Engine& engine, ModuleHost& modules, Packager& packager,
                         std::filesystem::path scene)
    : engine_(engine), modules_(modules), session_(engine), history_(engine.scene()),
      fonts_(cinder::text::FontSet::engineDefault()), platform_(engine.window()),
      app_(platform_, fonts_, panels::editorTheme()),
      toolbar_(session_, history_, selection_, packager, modules, std::move(scene), app_),
      view_(session_, selection_, history_, engine, app_), explorer_(selection_, history_, engine.scene(), app_),
      properties_(selection_, history_, engine.scene(), app_), console_(app_) {
    app_.setRoot(panels::editorLayout(toolbar_, view_, explorer_, properties_, console_, tabs_));
    engine.renderer().setUiPaint([this](cinder::ui::ElementList& list) {
        const std::vector<cinder::platform::InputEvent> events = engine_.input().takeEvents();
        app_.processEvents(events);
        toolbar_.update();
        view_.update();
        explorer_.update();
        properties_.update();
        console_.update();
        app_.paint(list);
        view_.afterPaint();
    });
}

EditorShell::~EditorShell() {
    engine_.renderer().setUiPaint(nullptr);
    app_.setRoot(nullptr);
}

bool EditorShell::applyPendingReload() {
    if (!modules_.reloadPending() || session_.state() != PlaySession::State::Edit || app_.isInteracting()
        || app_.hasPopups() || !modules_.reload()) {
        return false;
    }
    properties_.rebuild();
    return true;
}

}
