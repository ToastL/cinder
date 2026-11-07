#pragma once

#include "dev/History.hpp"
#include "dev/PlaySession.hpp"
#include "dev/Selection.hpp"
#include "dev/panels/Console.hpp"
#include "dev/panels/Explorer.hpp"
#include "dev/panels/Properties.hpp"
#include "dev/panels/SceneView.hpp"
#include "dev/panels/Toolbar.hpp"
#include "text/FontSet.hpp"
#include "ui/docking/TabManager.hpp"
#include "ui/framework/Application.hpp"
#include "ui/framework/WindowPlatform.hpp"

#include <filesystem>

namespace cinder::core { class Engine; }

namespace cinder::dev {

class ModuleHost;
class Packager;

class EditorShell {
public:
    EditorShell(cinder::core::Engine& engine, ModuleHost& modules, Packager& packager, std::filesystem::path scene);
    ~EditorShell();

    EditorShell(const EditorShell&) = delete;
    EditorShell& operator=(const EditorShell&) = delete;

    bool applyPendingReload();

    PlaySession& session() { return session_; }
    Selection& selection() { return selection_; }
    History& history() { return history_; }
    cinder::ui::Application& app() { return app_; }
    panels::Toolbar& toolbar() { return toolbar_; }
    panels::SceneView& view() { return view_; }
    panels::Explorer& explorer() { return explorer_; }
    panels::Properties& properties() { return properties_; }
    panels::Console& console() { return console_; }
    cinder::ui::TabManager& tabs() { return tabs_; }

private:
    cinder::core::Engine& engine_;
    ModuleHost& modules_;
    PlaySession session_;
    Selection selection_;
    History history_;
    cinder::text::FontSet fonts_;
    cinder::ui::WindowPlatform platform_;
    cinder::ui::Application app_;
    panels::Toolbar toolbar_;
    panels::SceneView view_;
    panels::Explorer explorer_;
    panels::Properties properties_;
    panels::Console console_;
    cinder::ui::TabManager tabs_;
};

}
