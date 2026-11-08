#include "Dev/Panels/Outliner.hpp"

#include "Dev/History.hpp"
#include "Dev/SceneDocument.hpp"
#include "Dev/Selection.hpp"
#include "Dev/Panels/EditorStyle.hpp"
#include "Reflect/Reflect.hpp"
#include "Scene/Actor.hpp"
#include "Scene/ClassRegistry.hpp"
#include "Scene/Component.hpp"
#include "Scene/Scene.hpp"
#include "Serial/Prefab.hpp"
#include "UI/Framework/Application.hpp"
#include "UI/Widgets/Border.hpp"
#include "UI/Widgets/BoxPanel.hpp"
#include "UI/Widgets/Button.hpp"
#include "UI/Widgets/ComboBox.hpp"
#include "UI/Widgets/Label.hpp"
#include "UI/Widgets/Menu.hpp"
#include "UI/Widgets/TextField.hpp"
#include "UI/Widgets/TreeView.hpp"

#include <filesystem>
#include <string_view>

namespace cinder::Dev::Panels {

using namespace cinder::UI;
using cinder::Scene::Actor;
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

}

Outliner::Outliner(Selection& selection, History& history, cinder::Scene::Scene& scene, Application& app)
    : selection_(selection), history_(history), scene_(scene), app_(app) {
    auto tree = make<TreeView>()
            .assign(tree_)
            .treeItemsSource([this] { return roots(); })
            .onGetChildren([this](ItemId item) { return childrenOf(item); })
            .onGetParent([this](ItemId item) { return parentOf(item); })
            .onGenerateRow([this](ItemId item) { return rowFor(item); })
            .isItemSelected([this](ItemId item) { return selectedActor() == static_cast<int>(item); })
            .onSelectionChanged([this](std::optional<ItemId> item, SelectInfo) {
                if (selection_.pick().armed()) {
                    Actor* offered = item ? scene_.actor(static_cast<int>(*item)) : nullptr;
                    if (offered == nullptr) selection_.pick().cancel();
                    else selection_.pick().offer(*offered);
                    return;
                }
                if (item) selection_.select(static_cast<int>(*item));
                else selection_.clear();
            })
            .onContextMenuOpening([this](std::optional<ItemId> item) { return contextMenu(item); })
            .onDragDetected([this](ItemId item) -> std::shared_ptr<DragDropOperation> {
                Actor* actor = scene_.actor(static_cast<int>(item));
                if (actor == nullptr || actor->prefabOwned()) return nullptr;
                return std::make_shared<ItemDragDrop>(std::vector<ItemId>{item}, actor->name());
            })
            .onCanAcceptDrop([this](const DragDropEvent& event, std::optional<ItemId> item,
                                    DropZone) -> std::optional<DropZone> {
                const auto* carried = dynamic_cast<const ItemDragDrop*>(event.operation.get());
                if (carried == nullptr || carried->items().empty()) return std::nullopt;
                Actor* dragged = scene_.actor(static_cast<int>(carried->items().front()));
                Actor* onto = item ? scene_.actor(static_cast<int>(*item)) : nullptr;
                if (dragged == nullptr || dragged == onto || dragged->isAncestorOf(onto)) return std::nullopt;
                if (onto == nullptr && dragged->attachParent() == nullptr) return std::nullopt;
                return DropZone::Onto;
            })
            .onAcceptDrop([this](const DragDropEvent& event, std::optional<ItemId> item, DropZone) {
                const auto* carried = dynamic_cast<const ItemDragDrop*>(event.operation.get());
                if (carried == nullptr || carried->items().empty()) return;
                attach(static_cast<int>(carried->items().front()),
                         item ? std::optional<int>(static_cast<int>(*item)) : std::nullopt);
            })
            .onKeyDownHandler([this](const KeyEvent& event) {
                if (event.key != keys::DELETE_KEY && event.key != keys::BACKSPACE) return false;
                if (const std::optional<int> selected = selectedActor()) remove(*selected);
                return true;
            });

    widget_ = make<VerticalBox>()
            + VerticalBox::slot().autoHeight()
                  [titleBar(make<HorizontalBox>()
                        + HorizontalBox::slot().autoWidth().padding(Margin(0.0f, 0.0f, 6.0f, 0.0f))
                              [make<ComboButton>()
                                       .buttonStyle("Button.Small")
                                       .hasDownArrow(false)
                                       .onGetMenuContent([this] {
                                           MenuBuilder menu;
                                           addInsertEntries(menu, selectedActor());
                                           return menu.build();
                                       })[make<Label>().text("+").toolTipText("Add an actor")]]
                        + HorizontalBox::slot().fill(1.0f)
                              [make<TextBox>().hintText("Filter").onTextChanged([this](const std::string& text) {
                                  filter_ = text;
                              })], Margin(6.0f, 3.0f))]
            + VerticalBox::slot().fill(1.0f)[tree];
}

std::optional<int> Outliner::selectedActor() const {
    const std::optional<int> id = selection_.id();
    if (!id) return std::nullopt;
    const cinder::Scene::Object* object = scene_.byId(*id);
    if (const auto* actor = dynamic_cast<const Actor*>(object)) return actor->id();
    if (const auto* component = dynamic_cast<const cinder::Scene::Component*>(object)) {
        if (component->owner() != nullptr) return component->owner()->id();
    }
    return std::nullopt;
}

bool Outliner::matches(const Actor& actor) const {
    if (filter_.empty()) return true;
    return containsIgnoreCase(actor.name(), filter_) || containsIgnoreCase(scene_.types().nameOf(actor), filter_);
}

void Outliner::gather(const Actor& actor, std::vector<ItemId>& out) const {
    if (actor.destroyed()) return;
    if (matches(actor)) out.push_back(static_cast<ItemId>(actor.id()));
    for (const Actor* child : actor.attachedActors()) gather(*child, out);
}

std::vector<ItemId> Outliner::roots() const {
    std::vector<ItemId> items;
    for (const Actor* root : scene_.roots()) {
        if (filter_.empty()) {
            if (!root->destroyed()) items.push_back(static_cast<ItemId>(root->id()));
        } else {
            gather(*root, items);
        }
    }
    return items;
}

std::vector<ItemId> Outliner::childrenOf(ItemId item) const {
    std::vector<ItemId> items;
    if (!filter_.empty()) return items;
    const Actor* actor = scene_.actor(static_cast<int>(item));
    if (actor == nullptr) return items;
    for (const Actor* child : actor->attachedActors()) {
        if (!child->destroyed()) items.push_back(static_cast<ItemId>(child->id()));
    }
    return items;
}

std::optional<ItemId> Outliner::parentOf(ItemId item) const {
    const Actor* actor = scene_.actor(static_cast<int>(item));
    if (actor == nullptr || actor->attachParent() == nullptr) return std::nullopt;
    return static_cast<ItemId>(actor->attachParent()->id());
}

std::shared_ptr<Widget> Outliner::rowFor(ItemId item) {
    const int id = static_cast<int>(item);
    const auto dimmed = [this, id] {
        const Actor* actor = scene_.actor(id);
        const bool lit = actor != nullptr && (actor->enabledInHierarchy() || selectedActor() == id);
        const Theme& theme = Application::get().theme();
        if (!lit) return theme.color("Color.ForegroundDim");
        const bool linked = !actor->prefab().empty() && selectedActor() != id;
        return linked ? theme.color("Color.Primary") : theme.color("Color.Foreground");
    };
    return make<HorizontalBox>()
           + HorizontalBox::slot().autoWidth().vAlign(VAlign::Center)
                 [make<Label>()
                          .text([this, id] {
                              const Actor* actor = scene_.actor(id);
                              return actor != nullptr ? actor->name() : std::string();
                          })
                          .colorAndOpacity(Attribute<Color>(dimmed))]
           + HorizontalBox::slot().fill(1.0f).vAlign(VAlign::Center).padding(Margin(6.0f, 0.0f, 0.0f, 0.0f))
                 [make<Label>()
                          .text([this, id] {
                              const Actor* actor = scene_.actor(id);
                              if (actor == nullptr) return std::string();
                              if (!actor->prefab().empty()) {
                                  return std::filesystem::path(actor->prefab()).stem().string();
                              }
                              const std::string detail(scene_.types().nameOf(*actor));
                              return detail == actor->name() ? std::string() : detail;
                          })
                          .textStyle("Label.Small")
                          .colorAndOpacity(themeColor("Color.ForegroundDim"))];
}

void Outliner::addInsertEntries(MenuBuilder& menu, std::optional<int> parent) {
    for (const auto& entry : scene_.types().registered()) {
        if (!entry.factory || entry.kind != cinder::Scene::ClassKind::Actor) continue;
        const std::string name = entry.name;
        menu.entry(name, [this, name, parent] { insert(name, parent); });
    }
    const std::vector<std::string> prefabs = SceneDocument::listPrefabs();
    if (prefabs.empty()) return;
    menu.subMenu("Prefab", [this, prefabs, parent](MenuBuilder& entries) {
        for (const std::string& prefab : prefabs) {
            entries.entry(prefab, [this, prefab, parent] { place(prefab, parent); });
        }
    });
}

std::shared_ptr<Widget> Outliner::contextMenu(std::optional<ItemId> item) {
    const std::optional<int> id = item ? std::optional<int>(static_cast<int>(*item)) : std::nullopt;
    MenuBuilder menu;
    menu.subMenu("Add Actor", [this, id](MenuBuilder& insertions) { addInsertEntries(insertions, id); });
    if (id) {
        menu.entry("Duplicate", [this, id] { duplicate(*id); })
                .entry("Delete", [this, id] { remove(*id); })
                .enabledIf([this, id] {
                    const Actor* actor = scene_.actor(*id);
                    return actor != nullptr && !actor->prefabOwned();
                });
        const Actor* actor = scene_.actor(*id);
        if (actor != nullptr && prefabs_.enabled) {
            const auto enabled = [this] { return prefabs_.available(); };
            menu.separator();
            if (prefabs_.save) {
                menu.entry("Save as Prefab...", [this, id] { prefabs_.save(*id); }).enabledIf(enabled);
            }
            if (!actor->prefab().empty()) {
                if (prefabs_.apply) {
                    menu.entry("Apply to Prefab", [this, id] { prefabs_.apply(*id); })
                            .enabledIf(enabled)
                            .toolTip("Write this instance into its prefab. Saves the scene and cannot be undone.");
                }
                if (prefabs_.revert) {
                    menu.entry("Revert to Prefab", [this, id] { prefabs_.revert(*id); }).enabledIf(enabled);
                }
                if (prefabs_.unpack) {
                    menu.entry("Unpack Prefab", [this, id] { prefabs_.unpack(*id); }).enabledIf(enabled);
                }
            }
        }
    }
    return menu.build();
}

void Outliner::insert(const std::string& className, std::optional<int> parent) {
    history_.touch("Add " + className, selection_.id());
    Actor* target = parent ? scene_.actor(*parent) : nullptr;
    if (Actor* actor = scene_.spawn(className, target)) {
        if (target != nullptr) tree_->setExpanded(static_cast<ItemId>(target->id()), true);
        selection_.select(actor->id(), true);
    }
}

void Outliner::place(const std::string& prefab, std::optional<int> parent) {
    history_.touch("Add " + std::filesystem::path(prefab).stem().string(), selection_.id());
    Actor* target = parent ? scene_.actor(*parent) : nullptr;
    if (Actor* actor = cinder::Serial::Prefab::instantiate(scene_, prefab, target)) {
        if (target != nullptr) tree_->setExpanded(static_cast<ItemId>(target->id()), true);
        selection_.select(actor->id(), true);
    }
}

void Outliner::duplicate(int id) {
    Actor* actor = scene_.actor(id);
    if (actor == nullptr) return;
    history_.touch("Duplicate " + actor->name(), selection_.id());
    if (Actor* copy = scene_.clone(*actor, actor->attachParent())) selection_.select(copy->id(), true);
}

void Outliner::remove(int id) {
    Actor* actor = scene_.actor(id);
    if (actor == nullptr || actor->prefabOwned()) return;
    history_.touch("Delete " + actor->name(), selection_.id());
    scene_.destroyNow(actor);
    selection_.clear();
}

void Outliner::attach(int id, std::optional<int> parent) {
    Actor* actor = scene_.actor(id);
    Actor* target = parent ? scene_.actor(*parent) : nullptr;
    if (actor == nullptr || actor == target || actor->isAncestorOf(target) || actor->prefabOwned()) return;
    history_.touch("Attach " + actor->name(), selection_.id());
    actor->attachTo(target);
    if (target != nullptr) tree_->setExpanded(static_cast<ItemId>(target->id()), true);
    selection_.select(id, true);
}

void Outliner::update() {
    if (!selection_.revealing()) return;
    if (const std::optional<int> id = selectedActor()) tree_->reveal(static_cast<ItemId>(*id));
    selection_.revealed();
}

}
