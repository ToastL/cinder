#pragma once

#include "Dev/History.hpp"
#include "Dev/PlaySession.hpp"
#include "Dev/Selection.hpp"
#include "Dev/Panels/Console.hpp"
#include "Dev/Panels/Explorer.hpp"
#include "Dev/Panels/Properties.hpp"
#include "Dev/Panels/SceneView.hpp"
#include "Dev/Panels/Toolbar.hpp"
#include "Text/FontSet.hpp"
#include "UI/Docking/TabManager.hpp"
#include "UI/Framework/Application.hpp"
#include "UI/Framework/WindowPlatform.hpp"

#include <filesystem>

namespace cinder::Core { class Engine; }

namespace cinder::Dev {

class ModuleHost;
class Packager;

class EditorShell {
public:
    EditorShell(cinder::Core::Engine& engine, ModuleHost& modules, Packager& packager, std::filesystem::path scene);
    ~EditorShell();

    EditorShell(const EditorShell&) = delete;
    EditorShell& operator=(const EditorShell&) = delete;

    bool applyPendingReload();

    PlaySession& session() { return session_; }
    Selection& selection() { return selection_; }
    History& history() { return history_; }
    cinder::UI::Application& app() { return app_; }
    Panels::Toolbar& toolbar() { return toolbar_; }
    Panels::SceneView& view() { return view_; }
    Panels::Explorer& explorer() { return explorer_; }
    Panels::Properties& properties() { return properties_; }
    Panels::Console& console() { return console_; }
    cinder::UI::TabManager& tabs() { return tabs_; }

private:
    cinder::Core::Engine& engine_;
    ModuleHost& modules_;
    PlaySession session_;
    Selection selection_;
    History history_;
    cinder::Text::FontSet fonts_;
    cinder::UI::WindowPlatform platform_;
    cinder::UI::Application app_;
    Panels::Toolbar toolbar_;
    Panels::SceneView view_;
    Panels::Explorer explorer_;
    Panels::Properties properties_;
    Panels::Console console_;
    cinder::UI::TabManager tabs_;
};

}
