#include "Dev/Panels/ScenePicker.hpp"

#include "Dev/Panels/Dialog.hpp"
#include "Reflect/Reflect.hpp"
#include "UI/Framework/Application.hpp"
#include "UI/Widgets/Border.hpp"
#include "UI/Widgets/BoxPanel.hpp"
#include "UI/Widgets/Label.hpp"
#include "UI/Widgets/SizeBox.hpp"
#include "UI/Widgets/TextField.hpp"
#include "UI/Widgets/TreeView.hpp"

#include <algorithm>
#include <string_view>
#include <utility>

namespace cinder::Dev::Panels {

using namespace cinder::UI;
namespace keys = cinder::Platform::keys;

namespace {

bool containsIgnoreCase(std::string_view text, std::string_view needle) {
    if (needle.size() > text.size()) return false;
    for (std::size_t i = 0; i + needle.size() <= text.size(); ++i) {
        std::size_t j = 0;
        while (j < needle.size()
               && cinder::Reflect::lowerAscii(text[i + j]) == cinder::Reflect::lowerAscii(needle[j])) {
            ++j;
        }
        if (j == needle.size()) return true;
    }
    return false;
}

ItemId itemFor(std::size_t index) { return static_cast<ItemId>(index + 1); }
std::size_t indexOf(ItemId item) { return static_cast<std::size_t>(item - 1); }

bool isEnter(int key) { return key == keys::ENTER || key == keys::KEYPAD_ENTER; }

}

void OpenSceneDialog::construct(const Args& args) {
    current_ = args.current_;
    onOpen_ = args.onOpen_;
    onCancel_ = args.onCancel_;

    auto field = make<TextBox>()
            .assign(field_)
            .hintText("Filter")
            .clearFocusOnCommit(false)
            .onTextChanged([this](const std::string& text) { filter(text); })
            .onKeyDownHandler([this](const KeyEvent& event) {
                if (event.key == keys::ESCAPE) {
                    if (onCancel_) onCancel_();
                    return true;
                }
                if (isEnter(event.key)) {
                    submit();
                    return true;
                }
                if (event.key == keys::DOWN || event.key == keys::UP) {
                    move(event.key == keys::DOWN ? 1 : -1);
                    return true;
                }
                return false;
            });

    auto list = make<TreeView>()
            .assign(list_)
            .treeItemsSource([this] {
                std::vector<ItemId> items;
                items.reserve(matches_.size());
                for (std::size_t i = 0; i < matches_.size(); ++i) items.push_back(itemFor(i));
                return items;
            })
            .onGenerateRow([this](ItemId item) -> std::shared_ptr<Widget> {
                const std::string name = indexOf(item) < matches_.size() ? matches_[indexOf(item)] : std::string();
                return make<Label>().text(name).colorAndOpacity(Attribute<Color>([this, name] {
                    const Theme& theme = Application::get().theme();
                    return theme.color(name == current_.get() ? "Color.ForegroundDim" : "Color.Foreground");
                }));
            })
            .isItemSelected([this](ItemId item) { return selected_ == indexOf(item); })
            .onSelectionChanged([this](std::optional<ItemId> item, SelectInfo) {
                if (item && indexOf(*item) < matches_.size()) selected_ = indexOf(*item);
                else selected_.reset();
            })
            .onMouseButtonDoubleClick([this](ItemId item) {
                if (indexOf(item) >= matches_.size()) return;
                selected_ = indexOf(item);
                submit();
            });

    auto body = make<VerticalBox>()
            + VerticalBox::slot().autoHeight().padding(Margin(0.0f, 0.0f, 0.0f, 6.0f))[field]
            + VerticalBox::slot().autoHeight()
                  [make<SizeBox>().heightOverride(LIST_HEIGHT)
                       [make<Overlay>()
                        + Overlay::slot()[list]
                        + Overlay::slot()
                              [make<Label>()
                                       .visibility([this] {
                                           return matches_.empty() ? Visibility::NoHitTest : Visibility::Collapsed;
                                       })
                                       .text([this] {
                                           return std::string(scenes_.empty() ? "No scenes under Content/"
                                                                              : "No scene matches the filter");
                                       })
                                       .colorAndOpacity(Attribute<Color>([] {
                                           return Application::get().theme().color("Color.ForegroundDim");
                                       }))]]];

    std::vector<DialogChoice> choices;
    choices.push_back({"Open", [this] { submit(); }, true, [this] { return chosen().has_value(); }});
    choices.push_back({"Cancel", [this] {
                           if (onCancel_) onCancel_();
                       }, false, {}});

    childSlot_.widget = make<Dialog>()
            .title("Open Scene")
            .message("Scenes under Content/")
            .body(body)
            .choices(std::move(choices))
            .onCancel([this] {
                if (onCancel_) onCancel_();
            });
}

void OpenSceneDialog::show(std::vector<std::string> scenes) {
    scenes_ = std::move(scenes);
    field_->setText("");
    filter("");
    const std::string current = current_.get();
    const auto it = std::find(matches_.begin(), matches_.end(), current);
    if (it != matches_.end()) {
        selected_ = static_cast<std::size_t>(it - matches_.begin());
        list_->requestScrollIntoView(itemFor(*selected_));
    }
}

std::shared_ptr<Widget> OpenSceneDialog::focusTarget() const { return field_->field(); }

std::optional<std::string> OpenSceneDialog::chosen() const {
    if (!selected_ || *selected_ >= matches_.size()) return std::nullopt;
    return matches_[*selected_];
}

void OpenSceneDialog::filter(const std::string& text) {
    matches_.clear();
    for (const std::string& scene : scenes_) {
        if (containsIgnoreCase(scene, text)) matches_.push_back(scene);
    }
    selected_ = matches_.empty() ? std::nullopt : std::optional<std::size_t>(0);
    list_->refresh();
}

void OpenSceneDialog::move(int delta) {
    if (matches_.empty()) return;
    const int last = static_cast<int>(matches_.size()) - 1;
    const int at = selected_ ? static_cast<int>(*selected_) + delta : 0;
    selected_ = static_cast<std::size_t>(std::clamp(at, 0, last));
    list_->requestScrollIntoView(itemFor(*selected_));
}

void OpenSceneDialog::submit() {
    const std::optional<std::string> scene = chosen();
    if (scene && onOpen_) onOpen_(*scene);
}

void SaveSceneDialog::construct(const Args& args) {
    onSave_ = args.onSave_;
    onCancel_ = args.onCancel_;
    extension_ = args.extension_;
    refuseExisting_ = args.refuseExisting_;

    auto field = make<TextBox>()
            .assign(field_)
            .hintText(args.hint_)
            .clearFocusOnCommit(false)
            .selectAllOnFocus(true)
            .onTextChanged([this](const std::string& text) { typed(text); })
            .onKeyDownHandler([this](const KeyEvent& event) {
                if (event.key == keys::ESCAPE) {
                    if (onCancel_) onCancel_();
                    return true;
                }
                if (isEnter(event.key)) {
                    submit();
                    return true;
                }
                return false;
            });

    auto body = make<VerticalBox>()
            + VerticalBox::slot().autoHeight().padding(Margin(0.0f, 0.0f, 0.0f, 6.0f))[field]
            + VerticalBox::slot().autoHeight()
                  [make<Label>()
                           .text([this] {
                               if (!resolved_.error.empty()) return resolved_.error;
                               const std::string name = SceneDocument::nameOf(resolved_.path);
                               return resolved_.exists ? "Replaces " + name : "Saves " + name;
                           })
                           .colorAndOpacity(Attribute<Color>([this] {
                               return Application::get().theme().color(
                                       resolved_.error.empty() ? "Color.ForegroundDim" : "Color.Error");
                           }))];

    std::vector<DialogChoice> choices;
    choices.push_back({"Save", [this] { submit(); }, true, [this] { return resolved_.error.empty(); }});
    choices.push_back({"Cancel", [this] {
                           if (onCancel_) onCancel_();
                       }, false, {}});

    childSlot_.widget = make<Dialog>()
            .title(args.title_)
            .message("A path under Content/")
            .body(body)
            .choices(std::move(choices))
            .onCancel([this] {
                if (onCancel_) onCancel_();
            });
}

void SaveSceneDialog::show(const std::string& name) {
    field_->setText(name);
    typed(name);
}

std::shared_ptr<Widget> SaveSceneDialog::focusTarget() const { return field_->field(); }

std::string SaveSceneDialog::text() const { return field_->text(); }

void SaveSceneDialog::typed(const std::string& text) {
    resolved_ = SceneDocument::resolve(text, extension_);
    if (refuseExisting_ && resolved_.error.empty() && resolved_.exists) {
        resolved_.error = SceneDocument::nameOf(resolved_.path) + " already exists";
    }
}

void SaveSceneDialog::submit() {
    if (!resolved_.error.empty()) return;
    if (onSave_) onSave_(resolved_.path);
}

}
