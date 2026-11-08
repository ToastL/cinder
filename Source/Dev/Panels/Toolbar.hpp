#pragma once

#include "UI/Core/Widget.hpp"
#include "UI/Framework/Commands.hpp"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace cinder::UI {
class Application;
class MenuBuilder;
}

namespace cinder::Dev {
enum class Platform : std::uint8_t;
class History;
class ModuleHost;
class Packager;
class PlaySession;
class SceneDocument;
class Selection;
}

namespace cinder::Dev::Panels {

class OpenSceneDialog;
class SaveSceneDialog;

class Toolbar {
public:
    Toolbar(PlaySession& session, History& history, Selection& selection, Packager& packager, ModuleHost& modules,
            SceneDocument& document, cinder::UI::Application& app);

    Toolbar(const Toolbar&) = delete;
    Toolbar& operator=(const Toolbar&) = delete;

    const std::shared_ptr<cinder::UI::Widget>& widget() const { return widget_; }
    const std::shared_ptr<cinder::UI::Widget>& prompt() const { return prompt_; }
    const std::shared_ptr<cinder::UI::CommandList>& commands() const { return commands_; }

    void update();
    void setWindowMenu(std::function<void(cinder::UI::MenuBuilder&)> fill) { windowMenu_ = std::move(fill); }
    void requestClose();
    void requestPackage(Platform platform);
    void requestNew();
    void requestOpen();
    void requestSaveAs();
    void requestSavePrefab(int actor);
    bool closeConfirmed() const { return closeConfirmed_; }
    bool prompting() const { return prompting_ != Prompt::None; }

    static const cinder::UI::Command& play();
    static const cinder::UI::Command& pause();
    static const cinder::UI::Command& step();
    static const cinder::UI::Command& newScene();
    static const cinder::UI::Command& openScene();
    static const cinder::UI::Command& save();
    static const cinder::UI::Command& saveAs();
    static const cinder::UI::Command& undo();
    static const cinder::UI::Command& redo();
    static const cinder::UI::Command& compile();

private:
    enum class Prompt : std::uint8_t { None, Unsaved, Packaging, OpenScene, SaveAs, SavePrefab };

    struct Pending {
        std::string verb;
        std::function<void()> action;
    };

    bool editing() const;
    bool write();
    void open(Prompt prompt);
    void guard(std::string verb, std::function<void()> action);
    void proceed();
    void confirmClose();
    void saveAsChosen(const std::filesystem::path& path);
    std::shared_ptr<cinder::UI::Widget> promptFocus() const;
    void bind();
    void build();
    std::shared_ptr<cinder::UI::Widget> buildMenus();
    void buildPrompts();

    PlaySession& session_;
    History& history_;
    Selection& selection_;
    Packager& packager_;
    ModuleHost& modules_;
    SceneDocument& document_;
    cinder::UI::Application& app_;
    std::shared_ptr<cinder::UI::CommandList> commands_;
    std::shared_ptr<cinder::UI::Widget> widget_;
    std::shared_ptr<cinder::UI::Widget> prompt_;
    std::shared_ptr<cinder::UI::Widget> unsavedPrompt_;
    std::shared_ptr<cinder::UI::Widget> packagingPrompt_;
    std::shared_ptr<OpenSceneDialog> openPrompt_;
    std::shared_ptr<SaveSceneDialog> saveAsPrompt_;
    std::shared_ptr<SaveSceneDialog> savePrefabPrompt_;
    int prefabSource_ = 0;
    std::function<void(cinder::UI::MenuBuilder&)> windowMenu_;
    Prompt prompting_ = Prompt::None;
    Prompt shown_ = Prompt::None;
    std::optional<Pending> pending_;
    bool focusPrompt_ = false;
    bool closeConfirmed_ = false;
};

}
