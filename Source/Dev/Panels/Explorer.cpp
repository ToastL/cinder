#include "Dev/Panels/Explorer.hpp"

#include "Dev/History.hpp"
#include "Dev/Selection.hpp"
#include "Dev/Panels/EditorStyle.hpp"
#include "Reflect/Reflect.hpp"
#include "Scene/Node.hpp"
#include "Scene/NodeTypes.hpp"
#include "Scene/Scene.hpp"
#include "UI/Framework/Application.hpp"
#include "UI/Widgets/Border.hpp"
#include "UI/Widgets/BoxPanel.hpp"
#include "UI/Widgets/Button.hpp"
#include "UI/Widgets/ComboBox.hpp"
#include "UI/Widgets/Label.hpp"
#include "UI/Widgets/Menu.hpp"
#include "UI/Widgets/TextField.hpp"
#include "UI/Widgets/TreeView.hpp"

#include <string_view>

namespace cinder::Dev::Panels {

using namespace cinder::UI;
using cinder::Scene::Node;
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

Explorer::Explorer(Selection& selection, History& history, cinder::Scene::Scene& scene, Application& app)
    : selection_(selection), history_(history), scene_(scene), app_(app) {
    auto tree = make<TreeView>()
            .assign(tree_)
            .treeItemsSource([this] { return roots(); })
            .onGetChildren([this](ItemId item) { return childrenOf(item); })
            .onGetParent([this](ItemId item) { return parentOf(item); })
            .onGenerateRow([this](ItemId item) { return rowFor(item); })
            .isItemSelected([this](ItemId item) { return selection_.selected(static_cast<int>(item)); })
            .onSelectionChanged([this](std::optional<ItemId> item, SelectInfo) {
                if (item) selection_.select(static_cast<int>(*item));
                else selection_.clear();
            })
            .onContextMenuOpening([this](std::optional<ItemId> item) { return contextMenu(item); })
            .onDragDetected([this](ItemId item) -> std::shared_ptr<DragDropOperation> {
                Node* node = scene_.byId(static_cast<int>(item));
                if (node == nullptr) return nullptr;
                return std::make_shared<ItemDragDrop>(std::vector<ItemId>{item}, node->name());
            })
            .onCanAcceptDrop([this](const DragDropEvent& event, std::optional<ItemId> item,
                                    DropZone) -> std::optional<DropZone> {
                const auto* carried = dynamic_cast<const ItemDragDrop*>(event.operation.get());
                if (carried == nullptr || carried->items().empty()) return std::nullopt;
                Node* dragged = scene_.byId(static_cast<int>(carried->items().front()));
                Node* onto = item ? scene_.byId(static_cast<int>(*item)) : nullptr;
                if (dragged == nullptr || dragged == onto || dragged->isAncestorOf(onto)) return std::nullopt;
                if (onto == nullptr && dragged->parent() == nullptr) return std::nullopt;
                return DropZone::Onto;
            })
            .onAcceptDrop([this](const DragDropEvent& event, std::optional<ItemId> item, DropZone) {
                const auto* carried = dynamic_cast<const ItemDragDrop*>(event.operation.get());
                if (carried == nullptr || carried->items().empty()) return;
                reparent(static_cast<int>(carried->items().front()),
                         item ? std::optional<int>(static_cast<int>(*item)) : std::nullopt);
            })
            .onKeyDownHandler([this](const KeyEvent& event) {
                if (event.key != keys::DELETE_KEY && event.key != keys::BACKSPACE) return false;
                if (const std::optional<int> selected = selection_.id()) remove(*selected);
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
                                           addInsertEntries(menu, selection_.id());
                                           return menu.build();
                                       })[make<Label>().text("+").toolTipText("Insert a node")]]
                        + HorizontalBox::slot().fill(1.0f)
                              [make<TextBox>().hintText("Filter").onTextChanged([this](const std::string& text) {
                                  filter_ = text;
                              })], Margin(6.0f, 3.0f))]
            + VerticalBox::slot().fill(1.0f)[tree];
}

bool Explorer::matches(const Node& node) const {
    if (filter_.empty()) return true;
    return containsIgnoreCase(node.name(), filter_) || containsIgnoreCase(scene_.types().nameOf(node), filter_);
}

void Explorer::gather(const Node& node, std::vector<ItemId>& out) const {
    if (node.destroyed()) return;
    if (matches(node)) out.push_back(static_cast<ItemId>(node.id()));
    for (const Node* child : node.children()) gather(*child, out);
}

std::vector<ItemId> Explorer::roots() const {
    std::vector<ItemId> items;
    for (const Node* root : scene_.roots()) {
        if (filter_.empty()) {
            if (!root->destroyed()) items.push_back(static_cast<ItemId>(root->id()));
        } else {
            gather(*root, items);
        }
    }
    return items;
}

std::vector<ItemId> Explorer::childrenOf(ItemId item) const {
    std::vector<ItemId> items;
    if (!filter_.empty()) return items;
    const Node* node = scene_.byId(static_cast<int>(item));
    if (node == nullptr) return items;
    for (const Node* child : node->children()) {
        if (!child->destroyed()) items.push_back(static_cast<ItemId>(child->id()));
    }
    return items;
}

std::optional<ItemId> Explorer::parentOf(ItemId item) const {
    const Node* node = scene_.byId(static_cast<int>(item));
    if (node == nullptr || node->parent() == nullptr) return std::nullopt;
    return static_cast<ItemId>(node->parent()->id());
}

std::shared_ptr<Widget> Explorer::rowFor(ItemId item) {
    const int id = static_cast<int>(item);
    const auto dimmed = [this, id] {
        const Node* node = scene_.byId(id);
        const bool lit = node != nullptr && (node->enabledInHierarchy() || selection_.selected(id));
        const Theme& theme = Application::get().theme();
        return lit ? theme.color("Color.Foreground") : theme.color("Color.ForegroundDim");
    };
    return make<HorizontalBox>()
           + HorizontalBox::slot().autoWidth().vAlign(VAlign::Center)
                 [make<Label>()
                          .text([this, id] {
                              const Node* node = scene_.byId(id);
                              return node != nullptr ? node->name() : std::string();
                          })
                          .colorAndOpacity(Attribute<Color>(dimmed))]
           + HorizontalBox::slot().fill(1.0f).vAlign(VAlign::Center).padding(Margin(6.0f, 0.0f, 0.0f, 0.0f))
                 [make<Label>()
                          .text([this, id] {
                              const Node* node = scene_.byId(id);
                              if (node == nullptr) return std::string();
                              const std::string detail(scene_.types().nameOf(*node));
                              return detail == node->name() ? std::string() : detail;
                          })
                          .textStyle("Label.Small")
                          .colorAndOpacity(themeColor("Color.ForegroundDim"))];
}

void Explorer::addInsertEntries(MenuBuilder& menu, std::optional<int> parent) {
    for (const auto& entry : scene_.types().registered()) {
        if (!entry.factory) continue;
        const std::string name = entry.name;
        menu.entry(name, [this, name, parent] { insert(name, parent); });
    }
}

std::shared_ptr<Widget> Explorer::contextMenu(std::optional<ItemId> item) {
    const std::optional<int> id = item ? std::optional<int>(static_cast<int>(*item)) : std::nullopt;
    MenuBuilder menu;
    menu.subMenu("Insert", [this, id](MenuBuilder& insertions) { addInsertEntries(insertions, id); });
    if (id) {
        menu.entry("Duplicate", [this, id] { duplicate(*id); })
                .entry("Delete", [this, id] { remove(*id); });
    }
    return menu.build();
}

void Explorer::insert(const std::string& className, std::optional<int> parent) {
    history_.touch("Insert " + className, selection_.id());
    Node* target = parent ? scene_.byId(*parent) : nullptr;
    if (Node* node = scene_.create(className, target)) {
        if (target != nullptr) tree_->setExpanded(static_cast<ItemId>(target->id()), true);
        selection_.select(node->id(), true);
    }
}

void Explorer::duplicate(int id) {
    Node* node = scene_.byId(id);
    if (node == nullptr) return;
    history_.touch("Duplicate " + node->name(), selection_.id());
    if (Node* copy = scene_.clone(*node, node->parent())) selection_.select(copy->id(), true);
}

void Explorer::remove(int id) {
    Node* node = scene_.byId(id);
    if (node == nullptr) return;
    history_.touch("Delete " + node->name(), selection_.id());
    scene_.destroyNow(node);
    selection_.clear();
}

void Explorer::reparent(int id, std::optional<int> parent) {
    Node* node = scene_.byId(id);
    Node* target = parent ? scene_.byId(*parent) : nullptr;
    if (node == nullptr || node == target || node->isAncestorOf(target)) return;
    history_.touch("Reparent " + node->name(), selection_.id());
    node->setParent(target);
    if (target != nullptr) tree_->setExpanded(static_cast<ItemId>(target->id()), true);
    selection_.select(id, true);
}

void Explorer::update() {
    if (!selection_.revealing()) return;
    if (const std::optional<int> id = selection_.id()) tree_->reveal(static_cast<ItemId>(*id));
    selection_.revealed();
}

}
