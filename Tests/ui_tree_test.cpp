#include <doctest/doctest.h>

#include "ui_harness.hpp"

#include "ui/widgets/BoxPanel.hpp"
#include "ui/widgets/Canvas.hpp"
#include "ui/widgets/Label.hpp"
#include "ui/widgets/Menu.hpp"
#include "ui/widgets/SpinBox.hpp"
#include "ui/widgets/TextField.hpp"
#include "ui/widgets/TreeView.hpp"

#include <map>
#include <string>
#include <vector>

using namespace cinder::ui;
using uitest::Harness;
using uitest::Probe;
namespace keys = cinder::platform::keys;
namespace modifiers = cinder::platform::modifiers;
namespace buttons = cinder::platform::buttons;

namespace {

struct Model {
    std::map<ItemId, std::vector<ItemId>> children;
    std::map<ItemId, ItemId> parents;
    std::vector<ItemId> roots;

    void add(ItemId item, ItemId parent) {
        children[parent].push_back(item);
        parents[item] = parent;
    }

    std::vector<ItemId> childrenOf(ItemId item) const {
        const auto found = children.find(item);
        return found == children.end() ? std::vector<ItemId>{} : found->second;
    }

    std::optional<ItemId> parentOf(ItemId item) const {
        const auto found = parents.find(item);
        return found == parents.end() ? std::nullopt : std::optional<ItemId>(found->second);
    }
};

Model sample() {
    Model model;
    model.roots = {1, 2};
    for (const ItemId root : model.roots) {
        for (ItemId i = 1; i <= 3; ++i) model.add(root * 10 + i, root);
    }
    model.add(111, 11);
    return model;
}

std::shared_ptr<Widget> rowFor(ItemId item) {
    return make<Label>().text("item " + std::to_string(item));
}

}

TEST_CASE("a list builds only the rows that fit on screen, and scrolling builds the next ones") {
    std::shared_ptr<TreeView> list;
    int built = 0;
    std::vector<ItemId> items;
    for (ItemId i = 1; i <= 1000; ++i) items.push_back(i);
    Harness ui(make<ListView>().assign(list).treeItemsSource([&] { return items; }).onGenerateRow([&](ItemId item) {
        ++built;
        return rowFor(item);
    }));
    CHECK(list->visibleCount() == 1000);
    CHECK(list->realizedCount() <= 20);
    CHECK(built <= 20);
    CHECK(list->rowFor(1));
    CHECK_FALSE(list->rowFor(500));

    ui.move(200.0f, 150.0f);
    for (int i = 0; i < 40; ++i) ui.wheel(-1.0f);
    ui.frame();
    CHECK_FALSE(list->rowFor(1));
    CHECK(list->realizedCount() <= 20);

    list->requestScrollIntoView(900);
    ui.frame();
    CHECK(list->rowFor(900));
}

TEST_CASE("a tree expands from its arrow alone, and from a double click") {
    const Model model = sample();
    std::shared_ptr<TreeView> tree;
    Harness ui(make<TreeView>()
                       .assign(tree)
                       .treeItemsSource([&] { return model.roots; })
                       .onGetChildren([&](ItemId item) { return model.childrenOf(item); })
                       .onGetParent([&](ItemId item) { return model.parentOf(item); })
                       .onGenerateRow(rowFor));
    CHECK(tree->visibleCount() == 2);

    const Rect first = ui.rectOf(tree->rowFor(1));
    ui.click(first.min.x + 40.0f, first.center().y);
    ui.frame();
    CHECK_FALSE(tree->isExpanded(1));
    CHECK(tree->isSelected(1));

    ui.platform.advance(1.0);
    ui.click(first.min.x + 7.0f, first.center().y);
    ui.frame();
    CHECK(tree->isExpanded(1));
    CHECK(tree->visibleCount() == 5);

    ui.platform.advance(1.0);
    ui.click(first.min.x + 40.0f, first.center().y);
    ui.platform.advance(0.1);
    ui.click(first.min.x + 40.0f, first.center().y);
    ui.frame();
    CHECK_FALSE(tree->isExpanded(1));
}

TEST_CASE("a tree selects with the mouse and the arrow keys, and reveals a hidden item") {
    const Model model = sample();
    std::vector<std::pair<std::optional<ItemId>, SelectInfo>> log;
    std::shared_ptr<TreeView> tree;
    Harness ui(make<TreeView>()
                       .assign(tree)
                       .treeItemsSource([&] { return model.roots; })
                       .onGetChildren([&](ItemId item) { return model.childrenOf(item); })
                       .onGetParent([&](ItemId item) { return model.parentOf(item); })
                       .onGenerateRow(rowFor)
                       .onSelectionChanged([&](std::optional<ItemId> item, SelectInfo how) { log.emplace_back(item, how); }));

    const Rect first = ui.rectOf(tree->rowFor(1));
    ui.click(first.center().x, first.center().y);
    REQUIRE(log.size() == 1);
    CHECK(log.back().first == std::optional<ItemId>(1));
    CHECK(log.back().second == SelectInfo::Mouse);
    CHECK(ui.app.focused() == tree);

    ui.key(keys::DOWN);
    CHECK(tree->isSelected(2));
    CHECK(log.back().second == SelectInfo::Keyboard);
    ui.key(keys::UP);
    CHECK(tree->isSelected(1));

    ui.key(keys::RIGHT);
    ui.frame();
    CHECK(tree->isExpanded(1));
    ui.key(keys::DOWN);
    CHECK(tree->isSelected(11));
    ui.key(keys::LEFT);
    CHECK(tree->isSelected(1));

    tree->reveal(111);
    ui.frame();
    CHECK(tree->isExpanded(11));
    CHECK(tree->rowFor(111));

    ui.platform.advance(1.0);
    ui.click(200.0f, 290.0f);
    CHECK_FALSE(log.back().first.has_value());
}

TEST_CASE("a right click selects the row it lands on and opens the tree's context menu") {
    const Model model = sample();
    std::optional<ItemId> asked;
    std::shared_ptr<TreeView> tree;
    Harness ui(make<TreeView>()
                       .assign(tree)
                       .treeItemsSource([&] { return model.roots; })
                       .onGetChildren([&](ItemId item) { return model.childrenOf(item); })
                       .onGenerateRow(rowFor)
                       .onContextMenuOpening([&](std::optional<ItemId> item) -> std::shared_ptr<Widget> {
                           asked = item;
                           MenuBuilder menu;
                           menu.entry("Delete", [] {});
                           return menu.build();
                       }));
    const Rect second = ui.rectOf(tree->rowFor(2));
    ui.move(second.center().x, second.center().y);
    ui.press(buttons::RIGHT);
    ui.release(buttons::RIGHT);
    CHECK(tree->isSelected(2));
    CHECK(asked == std::optional<ItemId>(2));
    CHECK(ui.app.hasPopups());

    ui.frame();
    ui.platform.advance(1.0);
    ui.click(200.0f, 290.0f);
    CHECK_FALSE(ui.app.hasPopups());
    ui.platform.advance(1.0);
    ui.move(200.0f, 290.0f);
    ui.press(buttons::RIGHT);
    ui.release(buttons::RIGHT);
    CHECK_FALSE(asked.has_value());
}

TEST_CASE("dragging a row carries an operation with a decorator, and a drop reparents") {
    Model model = sample();
    std::shared_ptr<TreeView> tree;
    std::vector<std::string> log;
    Harness ui(make<TreeView>()
                       .assign(tree)
                       .treeItemsSource([&] { return model.roots; })
                       .onGetChildren([&](ItemId item) { return model.childrenOf(item); })
                       .onGenerateRow(rowFor)
                       .onDragDetected([&](ItemId item) -> std::shared_ptr<DragDropOperation> {
                           return std::make_shared<ItemDragDrop>(std::vector<ItemId>{item}, "item " + std::to_string(item));
                       })
                       .onCanAcceptDrop([&](const DragDropEvent& event, std::optional<ItemId> item, DropZone) -> std::optional<DropZone> {
                           const auto* carried = dynamic_cast<const ItemDragDrop*>(event.operation.get());
                           if (carried == nullptr) return std::nullopt;
                           if (item && carried->items().front() == *item) return std::nullopt;
                           return DropZone::Onto;
                       })
                       .onAcceptDrop([&](const DragDropEvent& event, std::optional<ItemId> item, DropZone) {
                           const auto* carried = dynamic_cast<const ItemDragDrop*>(event.operation.get());
                           log.push_back(std::to_string(carried->items().front()) + " -> "
                                         + (item ? std::to_string(*item) : std::string("root")));
                       }));

    const Rect first = ui.rectOf(tree->rowFor(1));
    const Rect second = ui.rectOf(tree->rowFor(2));
    ui.move(first.center().x, first.center().y);
    ui.press();
    ui.move(first.center().x + 10.0f, first.center().y + 4.0f);
    REQUIRE(ui.app.dragDrop());
    CHECK(ui.app.isInteracting());
    ui.frame();
    ui.move(second.center().x, second.center().y);
    CHECK(tree->dropZoneFor(2) == std::optional<DropZone>(DropZone::Onto));
    ui.release();
    REQUIRE(log.size() == 1);
    CHECK(log.back() == "1 -> 2");
    CHECK_FALSE(ui.app.dragDrop());
    CHECK_FALSE(tree->dropZoneFor(2));

    ui.platform.advance(1.0);
    ui.move(second.center().x, second.center().y);
    ui.press();
    ui.move(second.center().x + 12.0f, second.center().y);
    REQUIRE(ui.app.dragDrop());
    ui.move(200.0f, 290.0f);
    ui.release();
    REQUIRE(log.size() == 2);
    CHECK(log.back() == "2 -> root");

    ui.platform.advance(1.0);
    ui.move(first.center().x, first.center().y);
    ui.press();
    ui.move(first.center().x + 12.0f, first.center().y);
    REQUIRE(ui.app.dragDrop());
    ui.key(keys::ESCAPE);
    CHECK_FALSE(ui.app.dragDrop());
    CHECK(log.size() == 2);
    ui.release();
}

TEST_CASE("a drag decorator is drawn at the cursor and is never hit") {
    std::shared_ptr<TreeView> tree;
    std::vector<ItemId> items = {1, 2};
    Harness ui(make<ListView>()
                       .assign(tree)
                       .treeItemsSource([&] { return items; })
                       .onGenerateRow(rowFor)
                       .onDragDetected([](ItemId item) -> std::shared_ptr<DragDropOperation> {
                           return std::make_shared<ItemDragDrop>(std::vector<ItemId>{item}, "Crate");
                       }));
    const Rect first = ui.rectOf(tree->rowFor(1));
    ui.move(first.center().x, first.center().y);
    ui.press();
    ui.move(first.center().x + 12.0f, first.center().y + 40.0f);
    REQUIRE(ui.app.dragDrop());
    ui.frame();

    const std::shared_ptr<Widget> decorator = ui.app.dragDrop()->decorator();
    REQUIRE(decorator);
    CHECK_FALSE(ui.app.grid().contains(decorator.get()));
    bool text = false;
    for (const Element& element : ui.list.elements()) {
        if (std::holds_alternative<TextElement>(element.shape)) text = true;
    }
    CHECK(text);
    ui.release();
}

TEST_CASE("Tab walks the fields in paint order, and steps into a spin box to type") {
    std::shared_ptr<TextBox> name;
    std::shared_ptr<TextBox> filter;
    std::shared_ptr<SpinBox> spin;
    double value = 2.0;
    Harness ui(make<VerticalBox>()
               + VerticalBox::slot().autoHeight()[make<TextBox>().assign(name).text("Box")]
               + VerticalBox::slot().autoHeight()[make<TextBox>().assign(filter).hintText("Filter")]
               + VerticalBox::slot().autoHeight()[make<SpinBox>().assign(spin).value([&] { return value; }).onValueChanged([&](double next) { value = next; })]);

    CHECK_FALSE(ui.app.focused());
    ui.key(keys::TAB);
    CHECK(ui.app.focused() == name->field());
    ui.key(keys::TAB);
    CHECK(ui.app.focused() == filter->field());
    ui.key(keys::TAB);
    CHECK(ui.app.focused() == spin->field());
    CHECK(spin->isEditing());
    ui.type(U"7");
    ui.key(keys::ENTER);
    CHECK(value == 7.0);

    ui.frame();
    ui.app.setFocus(filter->field());
    ui.key(keys::TAB, modifiers::SHIFT);
    CHECK(ui.app.focused() == name->field());
}

TEST_CASE("Tab inside a popup stays inside it") {
    std::shared_ptr<TextBox> outside;
    std::shared_ptr<TextBox> inside;
    std::shared_ptr<TextBox> other;
    Harness ui(make<VerticalBox>() + VerticalBox::slot().autoHeight()[make<TextBox>().assign(outside).text("Box")]);
    std::shared_ptr<Widget> popup = make<VerticalBox>()
            + VerticalBox::slot().autoHeight()[make<TextBox>().assign(inside).text("one")]
            + VerticalBox::slot().autoHeight()[make<TextBox>().assign(other).text("two")];
    ui.app.pushPopup(popup, Rect{{10.0f, 10.0f}, {200.0f, 30.0f}});
    ui.frame();

    ui.app.setFocus(inside->field());
    ui.key(keys::TAB);
    CHECK(ui.app.focused() == other->field());
    ui.key(keys::TAB);
    CHECK(ui.app.focused() == inside->field());
}
