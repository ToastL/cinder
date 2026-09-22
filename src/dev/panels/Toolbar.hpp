#pragma once

#include "ui/core/Widget.hpp"
#include "ui/framework/Commands.hpp"

#include <filesystem>
#include <memory>

namespace cinder::ui { class Application; }

namespace cinder::dev {
class History;
class PlaySession;
class Selection;
}

namespace cinder::dev::panels {

class Toolbar {
public:
    Toolbar(PlaySession& session, History& history, Selection& selection, std::filesystem::path scene,
            cinder::ui::Application& app);

    Toolbar(const Toolbar&) = delete;
    Toolbar& operator=(const Toolbar&) = delete;

    const std::shared_ptr<cinder::ui::Widget>& widget() const { return widget_; }
    const std::shared_ptr<cinder::ui::Widget>& prompt() const { return prompt_; }
    const std::shared_ptr<cinder::ui::CommandList>& commands() const { return commands_; }

    void update();
    void requestClose();
    bool closeConfirmed() const { return closeConfirmed_; }
    bool prompting() const { return prompting_; }

    static const cinder::ui::Command& play();
    static const cinder::ui::Command& pause();
    static const cinder::ui::Command& step();
    static const cinder::ui::Command& save();
    static const cinder::ui::Command& undo();
    static const cinder::ui::Command& redo();

private:
    bool editing() const;
    bool write();
    void bind();
    void build();

    PlaySession& session_;
    History& history_;
    Selection& selection_;
    std::filesystem::path scene_;
    cinder::ui::Application& app_;
    std::shared_ptr<cinder::ui::CommandList> commands_;
    std::shared_ptr<cinder::ui::Widget> widget_;
    std::shared_ptr<cinder::ui::Widget> prompt_;
    bool prompting_ = false;
    bool focusPrompt_ = false;
    bool closeConfirmed_ = false;
};

}
