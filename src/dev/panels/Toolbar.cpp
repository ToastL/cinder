#include "dev/panels/Toolbar.hpp"

#include "dev/History.hpp"
#include "dev/Packager.hpp"
#include "dev/PlaySession.hpp"
#include "dev/panels/Dialog.hpp"
#include "platform/Log.hpp"
#include "ui/framework/Application.hpp"
#include "ui/widgets/Border.hpp"
#include "ui/widgets/BoxPanel.hpp"
#include "ui/widgets/Button.hpp"
#include "ui/widgets/Label.hpp"
#include "ui/widgets/Menu.hpp"
#include "ui/widgets/SizeBox.hpp"

#include <exception>
#include <string>
#include <utility>

namespace cinder::dev::panels {

using namespace cinder::ui;
namespace keys = cinder::platform::keys;
namespace modifiers = cinder::platform::modifiers;

namespace {

std::shared_ptr<const Command> command(std::string name, std::string description, Shortcut shortcut) {
    return std::make_shared<const Command>(Command{name, name, std::move(description), shortcut});
}

const std::shared_ptr<const Command> PLAY = command("Play", "Play the scene, or stop it", Shortcut::primary(keys::letter('p')));
const std::shared_ptr<const Command> PAUSE =
        command("Pause", "Pause or resume the game", Shortcut::primary(keys::letter('p'), modifiers::SHIFT));
const std::shared_ptr<const Command> STEP =
        command("Step", "Run one fixed update", Shortcut::primary(keys::letter('p'), modifiers::ALT));
const std::shared_ptr<const Command> SAVE = command("Save", "Save the scene", Shortcut::primary(keys::letter('s')));
const std::shared_ptr<const Command> UNDO = command("Undo", "Undo the last edit", Shortcut::primary(keys::letter('z')));
const std::shared_ptr<const Command> REDO =
        command("Redo", "Redo the last undone edit", Shortcut::primary(keys::letter('z'), modifiers::SHIFT));

const char* label(PlaySession::State state) {
    switch (state) {
        case PlaySession::State::Edit: return "Edit";
        case PlaySession::State::Playing: return "Playing";
        case PlaySession::State::Paused: return "Paused";
    }
    return "";
}

BoxSlot gap(float width) { return HorizontalBox::slot().autoWidth()[make<Spacer>().size(glm::vec2(width, 0.0f))]; }

}

const Command& Toolbar::play() { return *PLAY; }
const Command& Toolbar::pause() { return *PAUSE; }
const Command& Toolbar::step() { return *STEP; }
const Command& Toolbar::save() { return *SAVE; }
const Command& Toolbar::undo() { return *UNDO; }
const Command& Toolbar::redo() { return *REDO; }

Toolbar::Toolbar(PlaySession& session, History& history, Selection& selection, Packager& packager,
                 std::filesystem::path scene, Application& app)
    : session_(session), history_(history), selection_(selection), packager_(packager), scene_(std::move(scene)),
      app_(app),
      commands_(std::make_shared<CommandList>()) {
    bind();
    build();
    app_.addCommands(commands_);
}

bool Toolbar::editing() const { return session_.state() == PlaySession::State::Edit; }

void Toolbar::bind() {
    commands_->map(PLAY, CommandAction{[this] { session_.togglePlay(); }, {}, {}});
    commands_->map(PAUSE, CommandAction{[this] { session_.togglePause(); }, [this] { return !editing(); }, {}});
    commands_->map(STEP, CommandAction{[this] { session_.step(); }, [this] { return !editing(); }, {}});
    commands_->map(SAVE, CommandAction{[this] { write(); }, [this] { return editing(); }, {}});
    commands_->map(UNDO, CommandAction{[this] { history_.undo(selection_); },
                                       [this] { return editing() && !app_.isInteracting(); }, {}});
    commands_->map(REDO, CommandAction{[this] { history_.redo(selection_); },
                                       [this] { return editing() && !app_.isInteracting(); }, {}});
}

void Toolbar::build() {
    const auto button = [this](std::shared_ptr<const Command> command, Attribute<std::string> text,
                               std::function<bool()> enabled, Attribute<std::string> tip) {
        auto list = commands_;
        return make<Button>()
                .text(std::move(text))
                .buttonStyle("Button.Toolbar")
                .isEnabled(std::move(enabled))
                .toolTipText(std::move(tip))
                .onClicked([list, command] {
                    list->execute(*command);
                    return Reply::handled();
                });
    };
    const auto shortcut = [](const std::shared_ptr<const Command>& command) {
        return command->description + "  " + command->shortcut.label();
    };

    const std::shared_ptr<const CommandList> list = commands_;
    std::shared_ptr<Widget> menus = make<MenuBar>()
            .menu("File", [list](MenuBuilder& menu) { menu.command(list, *SAVE); })
            .menu("Edit", [list](MenuBuilder& menu) { menu.command(list, *UNDO).command(list, *REDO); })
            .menu("Window", [this](MenuBuilder& menu) {
                if (windowMenu_) windowMenu_(menu);
            })
            .menu("Play", [this, list](MenuBuilder& menu) {
                menu.entry([this] { return std::string(editing() ? "Play" : "Stop"); }, [list] { list->execute(*PLAY); },
                           PLAY->shortcut.label())
                        .entry([this] { return std::string(session_.state() == PlaySession::State::Paused ? "Resume" : "Pause"); },
                               [list] { list->execute(*PAUSE); }, PAUSE->shortcut.label())
                        .enabledIf([this] { return !editing(); })
                        .command(list, *STEP);
            })
            .menu("Platforms", [this](MenuBuilder& menu) {
                menu.heading("Package Project");
                for (const Platform platform : platforms()) {
                    const std::string name(platformName(platform));
                    menu.entry([this, platform, name] {
                        if (packager_.current() == platform) return name + " (packaging...)";
                        return packager_.canPackage(platform) ? name : name + " (unavailable)";
                    }, [this, platform] { packager_.start(platform); })
                            .enabledIf([this, platform] { return packager_.canPackage(platform) && !packager_.busy(); });
                    const std::string reason = packager_.unavailableReason(platform);
                    menu.toolTip(reason.empty() ? "Package into " + packager_.outputDir(platform).string() : reason);
                }
            });

    widget_ = make<Border>()
            .brush([] { return Application::get().theme().get<Brush>("Brush.Toolbar"); })
            .padding(Margin(8.0f, 4.0f))
        [make<HorizontalBox>()
         + HorizontalBox::slot().autoWidth().vAlign(VAlign::Center)[menus]
         + gap(12.0f)
         + HorizontalBox::slot().autoWidth()
               [make<Button>()
                        .text([this] { return std::string(editing() ? "Play" : "Stop"); })
                        .buttonStyle([this] { return std::string(editing() ? "Button.Primary" : "Button"); })
                        .toolTipText(shortcut(PLAY))
                        .onClicked([this] {
                            session_.togglePlay();
                            return Reply::handled();
                        })]
         + gap(4.0f)
         + HorizontalBox::slot().autoWidth()[button(PAUSE, [this] {
               return std::string(session_.state() == PlaySession::State::Paused ? "Resume" : "Pause");
           }, [this] { return !editing(); }, shortcut(PAUSE))]
         + HorizontalBox::slot().autoWidth()[button(STEP, "Step", [this] { return !editing(); }, shortcut(STEP))]
         + gap(16.0f)
         + HorizontalBox::slot().autoWidth()[button(SAVE, "Save", [this] { return editing(); }, shortcut(SAVE))]
         + HorizontalBox::slot().autoWidth()[button(UNDO, "Undo", [this] { return editing() && history_.canUndo(); }, [this] {
               return history_.canUndo() ? "Undo " + history_.undoLabel() : std::string("Nothing to undo");
           })]
         + HorizontalBox::slot().autoWidth()[button(REDO, "Redo", [this] { return editing() && history_.canRedo(); }, [this] {
               return history_.canRedo() ? "Redo " + history_.redoLabel() : std::string("Nothing to redo");
           })]
         + HorizontalBox::slot().fill(1.0f)[make<Spacer>()]
         + HorizontalBox::slot().autoWidth().vAlign(VAlign::Center)
               [make<Label>()
                        .text([this] {
                            return std::string(label(session_.state())) + "  " + scene_.filename().string()
                                    + (history_.dirty() ? "*" : "");
                        })
                        .colorAndOpacity(Attribute<Color>([] { return Application::get().theme().color("Color.ForegroundDim"); }))]];

    std::vector<DialogChoice> choices;
    choices.push_back({"Save", [this] {
                           prompting_ = false;
                           closeConfirmed_ = write();
                       }, true});
    choices.push_back({"Don't Save", [this] {
                           prompting_ = false;
                           closeConfirmed_ = true;
                       }, false});
    choices.push_back({"Cancel", [this] { prompting_ = false; }, false});

    prompt_ = make<Dialog>()
            .visibility([this] { return prompting_ ? Visibility::Visible : Visibility::Collapsed; })
            .title("Unsaved Changes")
            .message("Save changes to " + scene_.filename().string() + " before closing?")
            .choices(std::move(choices))
            .onCancel([this] { prompting_ = false; });
}

void Toolbar::update() {
    packager_.update();
    history_.setEnabled(editing());
    history_.settle(app_.isInteracting());
    if (focusPrompt_) {
        app_.setFocus(prompt_);
        focusPrompt_ = false;
    }
}

void Toolbar::requestClose() {
    if (!history_.dirty()) {
        closeConfirmed_ = true;
        return;
    }
    prompting_ = true;
    focusPrompt_ = true;
}

bool Toolbar::write() {
    try {
        history_.save(scene_);
        cinder::platform::logInfo("[editor] saved %s\n", scene_.string().c_str());
        return true;
    } catch (const std::exception& e) {
        cinder::platform::logError("[editor] save failed: %s\n", e.what());
        return false;
    }
}

}
