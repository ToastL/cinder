#include "Dev/Panels/Toolbar.hpp"

#include "Dev/History.hpp"
#include "Dev/ModuleHost.hpp"
#include "Dev/Packager.hpp"
#include "Dev/PlaySession.hpp"
#include "Dev/SceneDocument.hpp"
#include "Dev/Selection.hpp"
#include "Dev/Panels/Dialog.hpp"
#include "Dev/Panels/EditorStyle.hpp"
#include "Dev/Panels/ScenePicker.hpp"
#include "Platform/Assets.hpp"
#include "Platform/Log.hpp"
#include "UI/Framework/Application.hpp"
#include "UI/Widgets/Border.hpp"
#include "UI/Widgets/BoxPanel.hpp"
#include "UI/Widgets/Button.hpp"
#include "UI/Widgets/Label.hpp"
#include "UI/Widgets/Menu.hpp"
#include "UI/Widgets/MenuBar.hpp"
#include "UI/Widgets/SizeBox.hpp"

#include <exception>
#include <string>
#include <utility>

namespace cinder::Dev::Panels {

using namespace cinder::UI;
namespace keys = cinder::Platform::keys;
namespace modifiers = cinder::Platform::modifiers;

namespace {

std::shared_ptr<const Command> command(std::string name, std::string description, Shortcut shortcut) {
    return std::make_shared<const Command>(Command{name, name, std::move(description), shortcut});
}

const std::shared_ptr<const Command> PLAY = command("Play", "Play the scene, or stop it", Shortcut::primary(keys::letter('p')));
const std::shared_ptr<const Command> PAUSE =
        command("Pause", "Pause or resume the game", Shortcut::primary(keys::letter('p'), modifiers::SHIFT));
const std::shared_ptr<const Command> STEP =
        command("Step", "Run one fixed update", Shortcut::primary(keys::letter('p'), modifiers::ALT));
const std::shared_ptr<const Command> CANCEL_PICK =
        command("Cancel Pick", "Stop picking a reference", Shortcut{keys::ESCAPE, 0});
const std::shared_ptr<const Command> NEW_SCENE =
        command("New Scene", "Start a new scene", Shortcut::primary(keys::letter('n')));
const std::shared_ptr<const Command> OPEN_SCENE =
        command("Open Scene...", "Open a scene from Content/", Shortcut::primary(keys::letter('o')));
const std::shared_ptr<const Command> SAVE = command("Save", "Save the scene", Shortcut::primary(keys::letter('s')));
const std::shared_ptr<const Command> SAVE_AS =
        command("Save As...", "Save the scene under a new name", Shortcut::primary(keys::letter('s'), modifiers::SHIFT));
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
const Command& Toolbar::newScene() { return *NEW_SCENE; }
const Command& Toolbar::openScene() { return *OPEN_SCENE; }
const Command& Toolbar::save() { return *SAVE; }
const Command& Toolbar::saveAs() { return *SAVE_AS; }
const Command& Toolbar::undo() { return *UNDO; }
const Command& Toolbar::redo() { return *REDO; }
const Command& Toolbar::compile() { return *BUILD; }

Toolbar::Toolbar(PlaySession& session, History& history, Selection& selection, Packager& packager,
                 ModuleHost& modules, SceneDocument& document, Application& app)
    : session_(session), history_(history), selection_(selection), packager_(packager), modules_(modules),
      document_(document),
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
    commands_->map(CANCEL_PICK, CommandAction{[this] { selection_.pick().cancel(); },
                                              [this] { return selection_.pick().armed(); }, {}});
    commands_->map(NEW_SCENE, CommandAction{[this] { requestNew(); }, [this] { return editing(); }, {}});
    commands_->map(OPEN_SCENE, CommandAction{[this] { requestOpen(); }, [this] { return editing(); }, {}});
    commands_->map(SAVE, CommandAction{[this] {
                                           if (document_.untitled()) requestSaveAs();
                                           else write();
                                       }, [this] { return editing(); }, {}});
    commands_->map(SAVE_AS, CommandAction{[this] { requestSaveAs(); }, [this] { return editing(); }, {}});
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

    std::shared_ptr<Widget> menus = buildMenus();

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
                            return std::string(label(session_.state())) + "  " + document_.displayName()
                                    + (history_.dirty() ? "*" : "");
                        })
                        .colorAndOpacity(themeColor("Color.ForegroundDim"))]];

    buildPrompts();
}

std::shared_ptr<Widget> Toolbar::buildMenus() {
    const std::shared_ptr<const CommandList> list = commands_;
    return make<MenuBar>()
            .menu("File", [list](MenuBuilder& menu) {
                menu.command(list, NEW_SCENE)
                        .command(list, OPEN_SCENE)
                        .separator()
                        .command(list, SAVE)
                        .command(list, SAVE_AS)
                        .separator()
                        .command(list, BUILD);
            })
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
}

void Toolbar::buildPrompts() {
    const auto dismiss = [this] {
        prompting_ = Prompt::None;
        pending_.reset();
    };

    std::vector<DialogChoice> unsaved;
    unsaved.push_back({"Save", [this] {
                           prompting_ = Prompt::None;
                           if (document_.untitled()) {
                               open(Prompt::SaveAs);
                               saveAsPrompt_->show("Scenes/untitled.scene");
                           } else if (write()) {
                               proceed();
                           } else {
                               pending_.reset();
                           }
                       }, true, {}});
    unsaved.push_back({"Don't Save", [this] {
                           prompting_ = Prompt::None;
                           proceed();
                       }, false, {}});
    unsaved.push_back({"Cancel", dismiss, false, {}});

    unsavedPrompt_ = make<Dialog>()
            .visibility([this] { return prompting_ == Prompt::Unsaved ? Visibility::Visible : Visibility::Collapsed; })
            .title("Unsaved Changes")
            .message([this] {
                return "Save changes to " + document_.displayName() + " "
                        + (pending_ ? pending_->verb : std::string("before closing?"));
            })
            .choices(std::move(unsaved))
            .onCancel(dismiss);

    std::vector<DialogChoice> packaging;
    packaging.push_back({"Stop and Close", [this] {
                             prompting_ = Prompt::None;
                             packager_.cancel();
                             confirmClose();
                         }, false, {}});
    packaging.push_back({"Keep Packaging", dismiss, true, {}});

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

    openPrompt_ = make<OpenSceneDialog>()
            .visibility([this] { return prompting_ == Prompt::OpenScene ? Visibility::Visible : Visibility::Collapsed; })
            .current([this] { return document_.untitled() ? std::string() : document_.displayName(); })
            .onOpen([this](const std::string& name) {
                prompting_ = Prompt::None;
                std::filesystem::path path;
                try {
                    path = cinder::Platform::contentPath(name);
                } catch (const std::exception& e) {
                    cinder::Platform::logError("[editor] %s\n", e.what());
                    return;
                }
                guard("before opening " + name + "?", [this, path] { document_.requestOpen(path); });
            })
            .onCancel(dismiss);

    saveAsPrompt_ = make<SaveSceneDialog>()
            .visibility([this] { return prompting_ == Prompt::SaveAs ? Visibility::Visible : Visibility::Collapsed; })
            .onSave([this](const std::filesystem::path& path) { saveAsChosen(path); })
            .onCancel(dismiss);

    prompt_ = make<Overlay>().visibility([this] { return prompting() ? Visibility::Visible : Visibility::Collapsed; })
            + Overlay::slot()[unsavedPrompt_] + Overlay::slot()[packagingPrompt_] + Overlay::slot()[openPrompt_]
            + Overlay::slot()[saveAsPrompt_];
}

void Toolbar::update() {
    packager_.update();
    history_.setEnabled(editing());
    if (!editing()) selection_.pick().cancel();
    if (shown_ != prompting_) {
        if (shown_ == Prompt::OpenScene || shown_ == Prompt::SaveAs) {
            const std::shared_ptr<Widget> focus = app_.focused();
            if (focus && (focus == openPrompt_->focusTarget() || focus == saveAsPrompt_->focusTarget())) {
                app_.clearFocus();
            }
        }
        shown_ = prompting_;
    }
    history_.settle(app_.isInteracting());
    if (focusPrompt_) {
        if (const std::shared_ptr<Widget> target = promptFocus()) app_.setFocus(target);
        focusPrompt_ = false;
    }
}

std::shared_ptr<Widget> Toolbar::promptFocus() const {
    switch (prompting_) {
        case Prompt::None: return nullptr;
        case Prompt::Unsaved: return unsavedPrompt_;
        case Prompt::Packaging: return packagingPrompt_;
        case Prompt::OpenScene: return openPrompt_->focusTarget();
        case Prompt::SaveAs: return saveAsPrompt_->focusTarget();
    }
    return nullptr;
}

void Toolbar::requestClose() {
    pending_.reset();
    prompting_ = Prompt::None;
    if (packager_.busy()) {
        open(Prompt::Packaging);
        return;
    }
    confirmClose();
}

void Toolbar::requestPackage(Platform platform) {
    if (!editing() || prompting()) return;
    guard("before packaging?", [this, platform] { packager_.start(platform); });
}

void Toolbar::requestNew() {
    if (!editing() || prompting()) return;
    guard("before starting a new scene?", [this] { document_.requestNew(); });
}

void Toolbar::requestOpen() {
    if (!editing() || prompting()) return;
    openPrompt_->show(SceneDocument::listScenes());
    open(Prompt::OpenScene);
}

void Toolbar::requestSaveAs() {
    if (!editing() || prompting()) return;
    pending_.reset();
    saveAsPrompt_->show(document_.untitled() ? std::string("Scenes/untitled.scene") : document_.displayName());
    open(Prompt::SaveAs);
}

void Toolbar::open(Prompt prompt) {
    prompting_ = prompt;
    focusPrompt_ = true;
}

void Toolbar::guard(std::string verb, std::function<void()> action) {
    if (!history_.dirty()) {
        action();
        return;
    }
    pending_ = Pending{std::move(verb), std::move(action)};
    open(Prompt::Unsaved);
}

void Toolbar::proceed() {
    if (std::optional<Pending> pending = std::exchange(pending_, std::nullopt)) {
        if (pending->action) pending->action();
    }
}

void Toolbar::confirmClose() {
    guard("before closing?", [this] { closeConfirmed_ = true; });
}

void Toolbar::saveAsChosen(const std::filesystem::path& path) {
    if (!document_.saveAs(path)) return;
    prompting_ = Prompt::None;
    proceed();
}

bool Toolbar::write() { return document_.save(); }

}
