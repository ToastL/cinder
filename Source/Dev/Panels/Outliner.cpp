#include "Dev/Panels/Outliner.hpp"

#include "Dev/AssetDragDrop.hpp"
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
            .isItemSelected([this](ItemId item) {
                const int target = static_cast<int>(item);
                for (int id : selection_.ids()) {
                    const cinder::Scene::Object* object = scene_.byId(id);
                    if (const auto* actor = dynamic_cast<const Actor*>(object)) {
                        if (actor->id() == target) return true;
                    } else if (const auto* component = dynamic_cast<const cinder::Scene::Component*>(object)) {
                        if (component->owner() != nullptr && component->owner()->id() == target) return true;
                    }
                }
                return false;
            })
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
            .onItemClicked([this](ItemId item, const PointerEvent& event) {
                return handleRowClick(item, event);
            })
            .onContextMenuOpening([this](std::optional<ItemId> item) { return contextMenu(item); })
            .onDragDetected([this](ItemId item) -> std::shared_ptr<DragDropOperation> {
                Actor* actor = scene_.actor(static_cast<int>(item));
                if (actor == nullptr || actor->prefabOwned()) return nullptr;
                std::vector<ItemId> items;
                std::string label = actor->name();
                if (selection_.isSelected(actor->id())) {
                    const std::vector<int> picked = removableSelection();
                    items.reserve(picked.size());
                    items.push_back(static_cast<ItemId>(actor->id()));
                    for (int id : picked) {
                        if (id == actor->id()) continue;
                        items.push_back(static_cast<ItemId>(id));
                    }
                    if (items.size() > 1) label = std::to_string(items.size()) + " actors";
                } else {
                    items.push_back(item);
                }
                return std::make_shared<ItemDragDrop>(std::move(items), std::move(label));
            })
            .onCanAcceptDrop([this](const DragDropEvent& event, std::optional<ItemId> item,
                                    DropZone) -> std::optional<DropZone> {
                if (AssetDragDrop::carried(event, AssetKind::Prefab) != nullptr) {
                    if (!item) return DropZone::Onto;
                    return scene_.actor(static_cast<int>(*item)) != nullptr ? std::optional(DropZone::Onto) : std::nullopt;
                }
                const auto* carried = dynamic_cast<const ItemDragDrop*>(event.operation.get());
                if (carried == nullptr || carried->items().empty()) return std::nullopt;
                Actor* dragged = scene_.actor(static_cast<int>(carried->items().front()));
                Actor* onto = item ? scene_.actor(static_cast<int>(*item)) : nullptr;
                if (dragged == nullptr || dragged == onto || dragged->isAncestorOf(onto)) return std::nullopt;
                if (onto == nullptr && dragged->attachParent() == nullptr) return std::nullopt;
                return DropZone::Onto;
            })
            .onAcceptDrop([this](const DragDropEvent& event, std::optional<ItemId> item, DropZone) {
                if (const AssetDragDrop* prefab = AssetDragDrop::carried(event, AssetKind::Prefab)) {
                    place(prefab->path(), item ? std::optional<int>(static_cast<int>(*item)) : std::nullopt);
                    return;
                }
                const auto* carried = dynamic_cast<const ItemDragDrop*>(event.operation.get());
                if (carried == nullptr || carried->items().empty()) return;
                attach(static_cast<int>(carried->items().front()),
                         item ? std::optional<int>(static_cast<int>(*item)) : std::nullopt);
            })
            .onKeyDownHandler([this](const KeyEvent& event) {
                if (event.key == keys::F2) {
                    if (const std::optional<int> id = selectedActor()) beginRename(*id);
                    return true;
                }
                if (event.key != keys::DELETE_KEY && event.key != keys::BACKSPACE) return false;
                const std::vector<int> ids = removableSelection();
                if (!ids.empty()) remove(ids);
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
    if (renaming_ == id) {
        std::shared_ptr<TextField> field;
        const Actor* actor = scene_.actor(id);
        const std::string original = actor != nullptr ? actor->name() : std::string();
        auto editor = make<TextField>()
                              .assign(field)
                              .text(original)
                              .selectAllOnFocus(true)
                              .revertOnEscape(true)
                              .onTextCommitted([this, id](const std::string& text, TextCommit how) {
                                  if (renaming_ != id) return;
                                  Actor* target = scene_.actor(id);
                                  if (target != nullptr && how == TextCommit::Enter) {
                                      std::string next = text;
                                      if (!next.empty() && next != target->name()) {
                                          history_.touch("Rename " + target->name(), selection_.id());
                                          target->setName(std::move(next));
                                      }
                                  }
                                  renaming_.reset();
                                  renameField_.reset();
                                  refreshPending_ = true;
                              });
        std::shared_ptr<Widget> row = editor;
        renameField_ = field;
        focusRename_ = true;
        return row;
    }
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
        const std::vector<int> group = removableSelection();
        const bool many = group.size() > 1 && selection_.isSelected(*id);
        const std::string suffix = many ? " (" + std::to_string(group.size()) + ")" : std::string();
        menu.entry("Duplicate" + suffix, [this, id, group, many] {
                    if (many) duplicate(group);
                    else duplicate(std::vector<int>{*id});
                })
                .enabledIf([this, id, group, many] {
                    if (many) return !group.empty();
                    const Actor* actor = scene_.actor(*id);
                    return actor != nullptr && !actor->prefabOwned();
                });
        menu.entry("Delete" + suffix, [this, id, group, many] {
                    if (many) remove(group);
                    else remove(std::vector<int>{*id});
                })
                .enabledIf([this, id, group, many] {
                    if (many) return !group.empty();
                    const Actor* actor = scene_.actor(*id);
                    return actor != nullptr && !actor->prefabOwned();
                });
        menu.entry("Rename", [this, id] { beginRename(*id); })
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

void Outliner::duplicate(const std::vector<int>& ids) {
    if (ids.empty()) return;
    const Actor* first = scene_.actor(ids.front());
    if (first == nullptr) return;
    const std::string label = ids.size() == 1 ? "Duplicate " + first->name()
                                              : "Duplicate " + std::to_string(ids.size()) + " actors";
    history_.touch(label, selection_.id());
    std::vector<int> copies;
    copies.reserve(ids.size());
    for (int id : ids) {
        Actor* actor = scene_.actor(id);
        if (actor == nullptr || actor->prefabOwned()) continue;
        if (Actor* copy = scene_.clone(*actor, actor->attachParent())) copies.push_back(copy->id());
    }
    if (!copies.empty()) selection_.setMany(copies, copies.front(), true);
}

void Outliner::remove(const std::vector<int>& ids) {
    if (ids.empty()) return;
    const Actor* first = scene_.actor(ids.front());
    const std::string label = ids.size() == 1 && first != nullptr
                                      ? "Delete " + first->name()
                                      : "Delete " + std::to_string(ids.size()) + " actors";
    history_.touch(label, selection_.id());
    for (int id : ids) {
        Actor* actor = scene_.actor(id);
        if (actor == nullptr || actor->prefabOwned()) continue;
        scene_.destroyNow(actor);
    }
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
    if (refreshPending_) {
        refreshPending_ = false;
        tree_->refresh();
    }
    if (renaming_ && scene_.actor(*renaming_) == nullptr) cancelRename();
    if (focusRename_) {
        if (std::shared_ptr<Widget> field = renameField_.lock()) {
            app_.setFocus(field);
            focusRename_ = false;
        }
    }
    if (!selection_.revealing()) return;
    if (const std::optional<int> id = selectedActor()) tree_->reveal(static_cast<ItemId>(*id));
    selection_.revealed();
}

void Outliner::beginRename(int id) {
    const Actor* actor = scene_.actor(id);
    if (actor == nullptr || actor->prefabOwned()) return;
    if (renaming_ == id) return;
    renaming_ = id;
    renameField_.reset();
    focusRename_ = true;
    tree_->refresh();
    tree_->requestScrollIntoView(static_cast<ItemId>(id));
}

void Outliner::cancelRename() {
    if (!renaming_) return;
    renaming_.reset();
    renameField_.reset();
    focusRename_ = false;
    tree_->refresh();
}

std::vector<int> Outliner::selectedActors() const {
    std::vector<int> out;
    for (int id : selection_.ids()) {
        if (scene_.actor(id) != nullptr) out.push_back(id);
    }
    return out;
}

std::vector<int> Outliner::removableSelection() const {
    std::vector<int> out;
    for (int id : selection_.ids()) {
        const Actor* actor = scene_.actor(id);
        if (actor != nullptr && !actor->prefabOwned()) out.push_back(id);
    }
    return out;
}

bool Outliner::handleRowClick(ItemId item, const PointerEvent& event) {
    if (selection_.pick().armed()) return false;
    const int id = static_cast<int>(item);
    if (event.shift() && selection_.id()) {
        const std::size_t anchor = tree_->indexOf(static_cast<ItemId>(*selection_.id()));
        const std::size_t target = tree_->indexOf(item);
        if (anchor < tree_->visibleCount() && target < tree_->visibleCount()) {
            const std::size_t lo = std::min(anchor, target);
            const std::size_t hi = std::max(anchor, target);
            std::vector<int> range;
            range.reserve(hi - lo + 1);
            for (std::size_t i = lo; i <= hi; ++i) {
                if (const std::optional<ItemId> at = tree_->itemAt(i)) range.push_back(static_cast<int>(*at));
            }
            selection_.setMany(range, *selection_.id());
            return true;
        }
    }
    if (event.primary()) {
        selection_.toggle(id);
        return true;
    }
    return false;
}

}
