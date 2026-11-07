#pragma once

#include "ui/core/Widget.hpp"
#include "ui/framework/Commands.hpp"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>

namespace cinder::ui {
class Application;
class MenuBuilder;
}

namespace cinder::dev {
enum class Platform : std::uint8_t;
class History;
class ModuleHost;
class Packager;
class PlaySession;
class Selection;
}

namespace cinder::dev::panels {

class Toolbar {
public:
    Toolbar(PlaySession& session, History& history, Selection& selection, Packager& packager, ModuleHost& modules,
            std::filesystem::path scene, cinder::ui::Application& app);

    Toolbar(const Toolbar&) = delete;
    Toolbar& operator=(const Toolbar&) = delete;

    const std::shared_ptr<cinder::ui::Widget>& widget() const { return widget_; }
    const std::shared_ptr<cinder::ui::Widget>& prompt() const { return prompt_; }
    const std::shared_ptr<cinder::ui::CommandList>& commands() const { return commands_; }

    void update();
    void setWindowMenu(std::function<void(cinder::ui::MenuBuilder&)> fill) { windowMenu_ = std::move(fill); }
    void requestClose();
    void requestPackage(Platform platform);
    bool closeConfirmed() const { return closeConfirmed_; }
    bool prompting() const { return prompting_ != Prompt::None; }

    static const cinder::ui::Command& play();
    static const cinder::ui::Command& pause();
    static const cinder::ui::Command& step();
    static const cinder::ui::Command& save();
    static const cinder::ui::Command& undo();
    static const cinder::ui::Command& redo();
    static const cinder::ui::Command& compile();

private:
    enum class Prompt : std::uint8_t { None, Unsaved, Packaging };

    bool editing() const;
    bool write();
    void open(Prompt prompt);
    void proceed();
    void confirmClose();
    void bind();
    void build();
    std::shared_ptr<cinder::ui::Widget> buildMenus();
    void buildPrompts();

    PlaySession& session_;
    History& history_;
    Selection& selection_;
    Packager& packager_;
    ModuleHost& modules_;
    std::filesystem::path scene_;
    cinder::ui::Application& app_;
    std::shared_ptr<cinder::ui::CommandList> commands_;
    std::shared_ptr<cinder::ui::Widget> widget_;
    std::shared_ptr<cinder::ui::Widget> prompt_;
    std::shared_ptr<cinder::ui::Widget> unsavedPrompt_;
    std::shared_ptr<cinder::ui::Widget> packagingPrompt_;
    std::function<void(cinder::ui::MenuBuilder&)> windowMenu_;
    Prompt prompting_ = Prompt::None;
    std::optional<Platform> packageAfterPrompt_;
    bool focusPrompt_ = false;
    bool closeConfirmed_ = false;
};

}
