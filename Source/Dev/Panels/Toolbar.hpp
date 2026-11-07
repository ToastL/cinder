#pragma once

#include "UI/Core/Widget.hpp"
#include "UI/Framework/Commands.hpp"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>

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
class Selection;
}

namespace cinder::Dev::Panels {

class Toolbar {
public:
    Toolbar(PlaySession& session, History& history, Selection& selection, Packager& packager, ModuleHost& modules,
            std::filesystem::path scene, cinder::UI::Application& app);

    Toolbar(const Toolbar&) = delete;
    Toolbar& operator=(const Toolbar&) = delete;

    const std::shared_ptr<cinder::UI::Widget>& widget() const { return widget_; }
    const std::shared_ptr<cinder::UI::Widget>& prompt() const { return prompt_; }
    const std::shared_ptr<cinder::UI::CommandList>& commands() const { return commands_; }

    void update();
    void setWindowMenu(std::function<void(cinder::UI::MenuBuilder&)> fill) { windowMenu_ = std::move(fill); }
    void requestClose();
    void requestPackage(Platform platform);
    bool closeConfirmed() const { return closeConfirmed_; }
    bool prompting() const { return prompting_ != Prompt::None; }

    static const cinder::UI::Command& play();
    static const cinder::UI::Command& pause();
    static const cinder::UI::Command& step();
    static const cinder::UI::Command& save();
    static const cinder::UI::Command& undo();
    static const cinder::UI::Command& redo();
    static const cinder::UI::Command& compile();

private:
    enum class Prompt : std::uint8_t { None, Unsaved, Packaging };

    bool editing() const;
    bool write();
    void open(Prompt prompt);
    void proceed();
    void confirmClose();
    void bind();
    void build();
    std::shared_ptr<cinder::UI::Widget> buildMenus();
    void buildPrompts();

    PlaySession& session_;
    History& history_;
    Selection& selection_;
    Packager& packager_;
    ModuleHost& modules_;
    std::filesystem::path scene_;
    cinder::UI::Application& app_;
    std::shared_ptr<cinder::UI::CommandList> commands_;
    std::shared_ptr<cinder::UI::Widget> widget_;
    std::shared_ptr<cinder::UI::Widget> prompt_;
    std::shared_ptr<cinder::UI::Widget> unsavedPrompt_;
    std::shared_ptr<cinder::UI::Widget> packagingPrompt_;
    std::function<void(cinder::UI::MenuBuilder&)> windowMenu_;
    Prompt prompting_ = Prompt::None;
    std::optional<Platform> packageAfterPrompt_;
    bool focusPrompt_ = false;
    bool closeConfirmed_ = false;
};

}
