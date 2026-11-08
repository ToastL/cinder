#include <doctest/doctest.h>

#include "ui_harness.hpp"

#include "UI/Widgets/Label.hpp"
#include "UI/Widgets/Menu.hpp"
#include "UI/Widgets/TileView.hpp"

#include <optional>
#include <string>
#include <vector>

using namespace cinder::UI;
using uitest::Harness;
namespace keys = cinder::Platform::keys;
namespace buttons = cinder::Platform::buttons;

namespace {

std::vector<ItemId> numbered(ItemId count) {
    std::vector<ItemId> items;
    for (ItemId i = 1; i <= count; ++i) items.push_back(i);
    return items;
}

std::shared_ptr<Widget> tileFor(ItemId item) { return make<Label>().text("tile " + std::to_string(item)); }

}

TEST_CASE("a tile view lays tiles out in as many columns as fit, and builds only the rows on screen") {
    std::shared_ptr<TileView> view;
    int built = 0;
    const std::vector<ItemId> items = numbered(500);
    Harness ui(make<TileView>()
                       .assign(view)
                       .tileSize(glm::vec2(88.0f, 104.0f))
                       .spacing(4.0f)
                       .itemsSource([&] { return items; })
                       .onGenerateTile([&](ItemId item) {
                           ++built;
                           return tileFor(item);
                       }));
    CHECK(view->itemCount() == 500);
    CHECK(view->columns() == 4);
    CHECK(view->realizedCount() <= 16);
    CHECK(built <= 16);
    REQUIRE(view->tileFor(1));
    CHECK_FALSE(view->tileFor(200));

    const Rect first = ui.rectOf(view->tileFor(1));
    const Rect second = ui.rectOf(view->tileFor(2));
    const Rect below = ui.rectOf(view->tileFor(5));
    CHECK(first.min == glm::vec2(0.0f));
    CHECK(second.min.x == doctest::Approx(92.0f));
    CHECK(below.min.y == doctest::Approx(108.0f));

    const std::shared_ptr<Tile> kept = view->tileFor(1);
    ui.frame();
    CHECK(view->tileFor(1) == kept);

    ui.move(200.0f, 150.0f);
    for (int i = 0; i < 40; ++i) ui.wheel(-1.0f);
    ui.frame();
    CHECK_FALSE(view->tileFor(1));
    CHECK(view->realizedCount() <= 16);

    ui.size = {200.0f, 300.0f};
    ui.frame();
    CHECK(view->columns() == 2);
}

TEST_CASE("clicking a tile selects it, a double click or Enter activates it, and arrows move by tile and row") {
    std::shared_ptr<TileView> view;
    std::vector<ItemId> activated;
    std::optional<ItemId> selected;
    const std::vector<ItemId> items = numbered(8);
    Harness ui(make<TileView>()
                       .assign(view)
                       .itemsSource([&] { return items; })
                       .onGenerateTile(tileFor)
                       .onSelectionChanged([&](std::optional<ItemId> item, SelectInfo) { selected = item; })
                       .onMouseButtonDoubleClick([&](ItemId item) { activated.push_back(item); }));
    REQUIRE(view->columns() == 4);

    const Rect two = ui.rectOf(view->tileFor(2));
    ui.click(two.center().x, two.center().y);
    CHECK(selected == ItemId{2});
    CHECK(view->isSelected(2));
    CHECK(ui.app.focused() == view);

    ui.key(keys::RIGHT);
    CHECK(selected == ItemId{3});
    ui.key(keys::DOWN);
    CHECK(selected == ItemId{7});
    ui.key(keys::UP);
    ui.key(keys::LEFT);
    CHECK(selected == ItemId{2});
    ui.key(keys::ENTER);
    CHECK(activated == std::vector<ItemId>{2});

    const Rect six = ui.rectOf(view->tileFor(6));
    ui.click(six.center().x, six.center().y);
    ui.click(six.center().x, six.center().y);
    CHECK(activated == std::vector<ItemId>{2, 6});

    ui.click(200.0f, 280.0f);
    CHECK_FALSE(selected.has_value());
}

TEST_CASE("a right press on a tile selects it and opens its menu, and on empty space opens the background menu") {
    std::shared_ptr<TileView> view;
    std::vector<std::optional<ItemId>> asked;
    const std::vector<ItemId> items = numbered(3);
    Harness ui(make<TileView>()
                       .assign(view)
                       .itemsSource([&] { return items; })
                       .onGenerateTile(tileFor)
                       .onContextMenuOpening([&](std::optional<ItemId> item) -> std::shared_ptr<Widget> {
                           asked.push_back(item);
                           MenuBuilder menu;
                           menu.entry("Open", [] {});
                           return menu.build();
                       }));
    const Rect three = ui.rectOf(view->tileFor(3));
    ui.click(three.center().x, three.center().y, buttons::RIGHT);
    REQUIRE(asked.size() == 1);
    CHECK(asked[0] == ItemId{3});
    CHECK(view->isSelected(3));
    CHECK(ui.app.hasPopups());

    ui.key(keys::ESCAPE);
    ui.frame();
    ui.click(300.0f, 250.0f, buttons::RIGHT);
    REQUIRE(asked.size() == 2);
    CHECK_FALSE(asked[1].has_value());
}

TEST_CASE("dragging a tile carries an operation, and a drop lands on a tile or on empty space") {
    std::shared_ptr<TileView> view;
    std::vector<std::optional<ItemId>> drops;
    const std::vector<ItemId> items = numbered(4);
    Harness ui(make<TileView>()
                       .assign(view)
                       .itemsSource([&] { return items; })
                       .onGenerateTile(tileFor)
                       .onDragDetected([](ItemId item) -> std::shared_ptr<DragDropOperation> {
                           if (item == 4) return nullptr;
                           return std::make_shared<ItemDragDrop>(std::vector<ItemId>{item}, "tile");
                       })
                       .onCanAcceptDrop([](const DragDropEvent& event, std::optional<ItemId> item) {
                           const auto* carried = dynamic_cast<const ItemDragDrop*>(event.operation.get());
                           return carried != nullptr && (!item || *item != carried->items().front());
                       })
                       .onAcceptDrop([&](const DragDropEvent&, std::optional<ItemId> item) { drops.push_back(item); }));

    const Rect one = ui.rectOf(view->tileFor(1));
    const Rect two = ui.rectOf(view->tileFor(2));
    ui.move(one.center().x, one.center().y);
    ui.press();
    ui.move(one.center().x + 12.0f, one.center().y + 4.0f);
    REQUIRE(ui.app.dragDrop());
    ui.move(two.center().x, two.center().y);
    CHECK(view->dropTargetIs(ItemId{2}));
    ui.release();
    CHECK_FALSE(ui.app.dragDrop());
    REQUIRE(drops.size() == 1);
    CHECK(drops[0] == ItemId{2});

    const Rect three = ui.rectOf(view->tileFor(3));
    ui.move(three.center().x, three.center().y);
    ui.press();
    ui.move(three.center().x + 12.0f, three.center().y + 4.0f);
    REQUIRE(ui.app.dragDrop());
    ui.move(300.0f, 250.0f);
    ui.release();
    REQUIRE(drops.size() == 2);
    CHECK_FALSE(drops[1].has_value());

    const Rect four = ui.rectOf(view->tileFor(4));
    ui.move(four.center().x, four.center().y);
    ui.press();
    ui.move(four.center().x + 12.0f, four.center().y + 4.0f);
    CHECK_FALSE(ui.app.dragDrop());
    ui.release();
}
