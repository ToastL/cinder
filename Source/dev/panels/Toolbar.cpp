#include "dev/panels/Toolbar.hpp"

#include "dev/History.hpp"
#include "dev/ModuleHost.hpp"
#include "dev/Packager.hpp"
#include "dev/PlaySession.hpp"
#include "dev/panels/Dialog.hpp"
#include "dev/panels/EditorStyle.hpp"
#include "platform/Log.hpp"
#include "ui/framework/Application.hpp"
#include "ui/widgets/Border.hpp"
#include "ui/widgets/BoxPanel.hpp"
#include "ui/widgets/Button.hpp"
#include "ui/widgets/Label.hpp"
#include "ui/widgets/Menu.hpp"
#include "ui/widgets/MenuBar.hpp"
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
const std::shared_ptr<const Command> BUILD =
        command("Build", "Compile the project's C++ and reload it", Shortcut::primary(keys::letter('b')));

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
const Command& Toolbar::compile() { return *BUILD; }

Toolbar::Toolbar(PlaySession& session, History& history, Selection& selection, Packager& packager,
                 ModuleHost& modules, std::filesystem::path scene, Application& app)
    : session_(session), history_(history), selection_(selection), packager_(packager), modules_(modules),
      scene_(std::move(scene)),
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
    commands_->map(BUILD, CommandAction{[this] { modules_.requestBuild(); },
                                        [this] { return modules_.available() && !modules_.building(); }, {}});
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
            .menu("File", [list](MenuBuilder& menu) { menu.command(list, SAVE).command(list, BUILD); })
            .menu("Edit", [list](MenuBuilder& menu) { menu.command(list, UNDO).command(list, REDO); })
            .menu("Window", [this](MenuBuilder& menu) {
                if (windowMenu_) windowMenu_(menu);
            })
            .menu("Play", [this, list](MenuBuilder& menu) {
                menu.entry([this] { return std::string(editing() ? "Play" : "Stop"); }, [list] { list->execute(*PLAY); },
                           PLAY->shortcut.label())
                        .entry([this] { return std::string(session_.state() == PlaySession::State::Paused ? "Resume" : "Pause"); },
                               [list] { list->execute(*PAUSE); }, PAUSE->shortcut.label())
                        .enabledIf([this] { return !editing(); })
                        .command(list, STEP);
            })
            .menu("Platforms", [this](MenuBuilder& menu) {
                menu.heading("Configuration");
                for (const Configuration configuration : {Configuration::Development, Configuration::Shipping}) {
                    menu.check(std::string(configurationName(configuration)),
                               [this, configuration] { packager_.setConfiguration(configuration); },
                               [this, configuration] { return packager_.configuration() == configuration; });
                }
                menu.separator();
                menu.heading("Package Project");
                for (const Platform platform : platforms()) {
                    const std::string name(platformName(platform));
                    menu.entry([this, platform, name] {
                        if (packager_.current() == platform) return name + " (packaging...)";
                        return packager_.canPackage(platform) ? name : name + " (unavailable)";
                    }, [this, platform] { requestPackage(platform); })
                            .enabledIf([this, platform] {
                                return editing() && packager_.canPackage(platform) && !packager_.busy();
                            });
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
                            commands_->execute(*PLAY);
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
         + gap(16.0f)
         + HorizontalBox::slot().autoWidth()[button(BUILD, "Build", [this] {
               return modules_.available() && !modules_.building();
           }, [this, shortcut] {
               return modules_.available() ? shortcut(BUILD) : std::string("cbt was not found next to the editor");
           })]
         + HorizontalBox::slot().autoWidth().vAlign(VAlign::Center).padding(Margin(6.0f, 0.0f, 0.0f, 0.0f))
               [make<Label>()
                        .text([this] { return modules_.status(); })
                        .colorAndOpacity(Attribute<Color>([this] {
                            return Application::get().theme().color(
                                    modules_.lastResult().errors > 0 ? "Color.Error" : "Color.ForegroundDim");
                        }))]
         + HorizontalBox::slot().fill(1.0f)[make<Spacer>()]
         + HorizontalBox::slot().autoWidth().vAlign(VAlign::Center)
               [make<Label>()
                        .text([this] {
                            return std::string(label(session_.state())) + "  " + scene_.filename().string()
                                    + (history_.dirty() ? "*" : "");
                        })
                        .colorAndOpacity(themeColor("Color.ForegroundDim"))]];

    const auto dismiss = [this] {
        prompting_ = Prompt::None;
        packageAfterPrompt_.reset();
    };

    std::vector<DialogChoice> unsaved;
    unsaved.push_back({"Save", [this] {
                           prompting_ = Prompt::None;
                           if (write()) {
                               proceed();
                           } else {
                               packageAfterPrompt_.reset();
                           }
                       }, true});
    unsaved.push_back({"Don't Save", [this] {
                           prompting_ = Prompt::None;
                           proceed();
                       }, false});
    unsaved.push_back({"Cancel", dismiss, false});

    unsavedPrompt_ = make<Dialog>()
            .visibility([this] { return prompting_ == Prompt::Unsaved ? Visibility::Visible : Visibility::Collapsed; })
            .title("Unsaved Changes")
            .message([this] {
                return "Save changes to " + scene_.filename().string()
                        + (packageAfterPrompt_ ? " before packaging?" : " before closing?");
            })
            .choices(std::move(unsaved))
            .onCancel(dismiss);

    std::vector<DialogChoice> packaging;
    packaging.push_back({"Stop and Close", [this] {
                             prompting_ = Prompt::None;
                             packager_.cancel();
                             confirmClose();
                         }, false});
    packaging.push_back({"Keep Packaging", dismiss, true});

    packagingPrompt_ = make<Dialog>()
            .visibility([this] { return prompting_ == Prompt::Packaging ? Visibility::Visible : Visibility::Collapsed; })
            .title("Packaging in Progress")
            .message([this] {
                const std::optional<Platform> current = packager_.current();
                const std::string name = current ? std::string(platformName(*current)) : std::string("The project");
                return name + " packaging is still running. Stop it and close?";
            })
            .choices(std::move(packaging))
            .onCancel(dismiss);

    prompt_ = make<Overlay>().visibility([this] { return prompting() ? Visibility::Visible : Visibility::Collapsed; })
            + Overlay::slot()[unsavedPrompt_] + Overlay::slot()[packagingPrompt_];
}

void Toolbar::update() {
    packager_.update();
    history_.setEnabled(editing());
    history_.settle(app_.isInteracting());
    if (focusPrompt_) {
        app_.setFocus(prompting_ == Prompt::Packaging ? packagingPrompt_ : unsavedPrompt_);
        focusPrompt_ = false;
    }
}

void Toolbar::requestClose() {
    packageAfterPrompt_.reset();
    if (packager_.busy()) {
        open(Prompt::Packaging);
        return;
    }
    confirmClose();
}

void Toolbar::requestPackage(Platform platform) {
    if (!editing() || prompting()) return;
    if (!history_.dirty()) {
        packager_.start(platform);
        return;
    }
    packageAfterPrompt_ = platform;
    open(Prompt::Unsaved);
}

void Toolbar::open(Prompt prompt) {
    prompting_ = prompt;
    focusPrompt_ = true;
}

void Toolbar::proceed() {
    if (packageAfterPrompt_) {
        const Platform platform = *packageAfterPrompt_;
        packageAfterPrompt_.reset();
        packager_.start(platform);
        return;
    }
    closeConfirmed_ = true;
}

void Toolbar::confirmClose() {
    if (!history_.dirty()) {
        closeConfirmed_ = true;
        return;
    }
    open(Prompt::Unsaved);
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
