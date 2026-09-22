#include <doctest/doctest.h>

#include "ui_harness.hpp"

#include "ui/framework/Commands.hpp"
#include "ui/widgets/Canvas.hpp"
#include "ui/widgets/ColorPicker.hpp"
#include "ui/widgets/ComboBox.hpp"
#include "ui/widgets/ExpandableArea.hpp"
#include "ui/widgets/Menu.hpp"
#include "ui/widgets/SpinBox.hpp"
#include "ui/widgets/TextField.hpp"
#include "ui/widgets/VectorInputBox.hpp"

#include <glm/geometric.hpp>

using namespace cinder::ui;
using uitest::Harness;
using uitest::Probe;
using uitest::count;
namespace keys = cinder::platform::keys;
namespace modifiers = cinder::platform::modifiers;

namespace {

std::shared_ptr<Widget> at(glm::vec2 position, glm::vec2 size, std::shared_ptr<Widget> content) {
    return make<Canvas>() + Canvas::slot().offset(Margin(position.x, position.y, size.x, size.y))[std::move(content)];
}

void clickAt(Harness& ui, glm::vec2 point) { ui.click(point.x, point.y); }

}

TEST_CASE("typed numbers are evaluated, not just parsed") {
    CHECK(evaluateNumber("2*(3+4)") == 14.0);
    CHECK(evaluateNumber(" -1.5 ") == -1.5);
    CHECK(evaluateNumber("1e3") == 1000.0);
    CHECK(evaluateNumber("10 / 4") == 2.5);
    CHECK(evaluateNumber("--2") == 2.0);
    CHECK_FALSE(evaluateNumber("1/0"));
    CHECK_FALSE(evaluateNumber("abc"));
    CHECK_FALSE(evaluateNumber("2 3"));
    CHECK_FALSE(evaluateNumber(""));
    CHECK_FALSE(evaluateNumber("(1"));
}

TEST_CASE("numbers display short, and edit at full precision") {
    CHECK(formatNumber(1.0, 3, false) == "1.0");
    CHECK(formatNumber(0.12345, 3, false) == "0.123");
    CHECK(formatNumber(-0.0001, 3, false) == "0.0");
    CHECK(formatNumber(1234567.0, 3, false) == "1234567.0");
    CHECK(formatNumber(2.6, 3, true) == "3");
    CHECK(exactNumber(static_cast<double>(0.1f), false) == "0.1");
    CHECK(exactNumber(0.1, false) == "0.1");
    CHECK(exactNumber(0.123456789, false) == "0.123456789");
    CHECK(exactNumber(-0.0, false) == "0");
    CHECK(exactNumber(41.7, true) == "42");
}

TEST_CASE("dragging a spin box moves its value by step per point, past the drag threshold") {
    double value = 1.0;
    int commits = 0;
    int begins = 0;
    std::shared_ptr<SpinBox> spin;
    Harness ui(at({10.0f, 10.0f}, {120.0f, 24.0f},
                  make<SpinBox>()
                          .assign(spin)
                          .value([&] { return value; })
                          .step(0.5)
                          .onValueChanged([&](double next) { value = next; })
                          .onValueCommitted([&](double, TextCommit) { ++commits; })
                          .onBeginSliderMovement([&] { ++begins; })));
    ui.move(50.0f, 20.0f);
    ui.press();
    ui.move(54.0f, 20.0f);
    CHECK(value == 1.0);
    ui.move(60.0f, 20.0f);
    CHECK(value == 1.0);
    CHECK(spin->isDragging());
    CHECK(begins == 1);
    ui.move(70.0f, 20.0f);
    CHECK(value == 6.0);
    ui.mods = modifiers::SHIFT;
    ui.move(71.0f, 20.0f);
    CHECK(value == 11.0);
    ui.mods = modifiers::ALT;
    ui.move(73.0f, 20.0f);
    CHECK(value == doctest::Approx(11.1));
    ui.mods = 0;
    ui.move(-300.0f, 20.0f);
    CHECK(value < 0.0);
    ui.release();
    CHECK(commits == 1);
    CHECK_FALSE(spin->isDragging());
    CHECK_FALSE(spin->isEditing());
}

TEST_CASE("a spin box clamps only to the bounds it declares, and an integral one never holds a fraction") {
    double value = 5.0;
    Harness ui(at({10.0f, 10.0f}, {120.0f, 24.0f},
                  make<SpinBox>()
                          .value([&] { return value; })
                          .minValue(0.0)
                          .maxValue(10.0)
                          .integral(true)
                          .step(0.3)
                          .onValueChanged([&](double next) { value = next; })));
    ui.move(50.0f, 20.0f);
    ui.press();
    ui.move(60.0f, 20.0f);
    ui.move(65.0f, 20.0f);
    CHECK(value == 7.0);
    ui.move(390.0f, 20.0f);
    CHECK(value == 10.0);
    ui.move(385.0f, 20.0f);
    CHECK(value == 9.0);
    ui.release();
}

TEST_CASE("clicking a spin box types into it, Enter commits the evaluated value and Escape reverts") {
    double value = static_cast<double>(0.1f);
    int changes = 0;
    std::shared_ptr<SpinBox> spin;
    Harness ui(at({10.0f, 10.0f}, {120.0f, 24.0f},
                  make<SpinBox>().assign(spin).value([&] { return value; }).onValueChanged([&](double next) {
                      value = next;
                      ++changes;
                  })));
    ui.click(50.0f, 20.0f);
    CHECK(spin->isEditing());
    CHECK(ui.app.focused() == spin->field());
    CHECK(spin->field()->text() == "0.1");
    ui.key(keys::ENTER);
    CHECK(changes == 0);
    CHECK(value == static_cast<double>(0.1f));
    ui.frame();
    CHECK_FALSE(spin->isEditing());

    ui.platform.advance(1.0);
    ui.click(50.0f, 20.0f);
    ui.type(U"2*3");
    ui.key(keys::ENTER);
    CHECK(value == 6.0);
    CHECK(changes == 1);

    ui.frame();
    ui.platform.advance(1.0);
    ui.click(50.0f, 20.0f);
    ui.type(U"99");
    ui.key(keys::ESCAPE);
    CHECK(value == 6.0);
    ui.frame();
    CHECK_FALSE(spin->isEditing());

    ui.platform.advance(1.0);
    ui.click(50.0f, 20.0f);
    ui.frame();
    CHECK(spin->isEditing());
    ui.platform.advance(1.0);
    ui.click(300.0f, 200.0f);
    ui.frame();
    CHECK_FALSE(spin->isEditing());
    CHECK(changes == 1);
}

TEST_CASE("a vector box edits one component at a time and colours its axes") {
    glm::dvec3 value(1.0, 2.0, 3.0);
    std::shared_ptr<VectorInputBox> vector;
    Harness ui(at({0.0f, 0.0f}, {300.0f, 24.0f},
                  make<VectorInputBox>()
                          .assign(vector)
                          .value([&](int index) { return value[index]; })
                          .step(1.0)
                          .onComponentChanged([&](int index, double next) { value[index] = next; })));
    REQUIRE(vector->components() == 3);
    const Rect y = ui.rectOf(vector->component(1));
    CHECK(y.min.x > 90.0f);
    ui.move(y.center().x, y.center().y);
    ui.press();
    ui.move(y.center().x + 10.0f, y.center().y);
    ui.move(y.center().x + 14.0f, y.center().y);
    ui.release();
    CHECK(value == glm::dvec3(1.0, 6.0, 3.0));
}

TEST_CASE("a combo box opens its options below it and picks one by mouse or keyboard") {
    std::string chosen = "box";
    std::shared_ptr<ComboBox> combo;
    Harness ui(at({10.0f, 10.0f}, {120.0f, 24.0f},
                  make<ComboBox>()
                          .assign(combo)
                          .options(std::vector<std::string>{"box", "sphere", "capsule"})
                          .selectedOption([&] { return chosen; })
                          .onSelectionChanged([&](const std::string& option) { chosen = option; })));
    ui.click(50.0f, 20.0f);
    REQUIRE(ui.app.hasPopups());
    ui.frame();
    auto menu = std::dynamic_pointer_cast<Menu>(combo->button()->anchor()->menu());
    REQUIRE(menu);
    CHECK(menu->highlighted() == 0);
    const Rect popup = *ui.app.popupRect(menu.get());
    CHECK(popup.min.y == 34.0f);
    CHECK(popup.width() >= 120.0f);

    clickAt(ui, ui.rectOf(menu->row(1)).center());
    CHECK(chosen == "sphere");
    CHECK_FALSE(ui.app.hasPopups());

    ui.frame();
    ui.platform.advance(1.0);
    ui.click(50.0f, 20.0f);
    ui.frame();
    menu = std::dynamic_pointer_cast<Menu>(combo->button()->anchor()->menu());
    REQUIRE(menu);
    CHECK(menu->highlighted() == 1);
    CHECK(ui.app.focused() == menu);
    ui.key(keys::DOWN);
    ui.key(keys::ENTER);
    CHECK(chosen == "capsule");
    CHECK_FALSE(ui.app.hasPopups());

    ui.frame();
    ui.platform.advance(1.0);
    ui.click(50.0f, 20.0f);
    ui.frame();
    ui.platform.advance(1.0);
    ui.click(50.0f, 20.0f);
    CHECK_FALSE(ui.app.hasPopups());
}

TEST_CASE("a menu skips disabled entries, checks, and opens submenus by hover and by key") {
    int saved = 0;
    bool snap = false;
    std::string inserted;
    MenuBuilder builder;
    builder.entry("Save", [&] { ++saved; }, "S")
            .entry("Locked", [&] { ++saved; })
            .enabledIf([] { return false; })
            .separator()
            .check("Snap", [&] { snap = !snap; }, [&] { return snap; })
            .subMenu("Insert", [&](MenuBuilder& sub) {
                sub.entry("Cube", [&] { inserted = "cube"; }).entry("Sphere", [&] { inserted = "sphere"; });
            });
    std::shared_ptr<Menu> menu = builder.build();
    Harness ui(make<Probe>().handles(false));
    PopupOptions options;
    options.placement = Placement::AtPoint;
    ui.app.pushPopup(menu, Rect::fromSize({20.0f, 20.0f}, glm::vec2(0.0f)), options);
    ui.frame();
    REQUIRE(menu->rowCount() == 4);

    clickAt(ui, ui.rectOf(menu->row(1)).center());
    CHECK(saved == 0);
    CHECK(ui.app.hasPopups());

    ui.key(keys::DOWN);
    CHECK(menu->highlighted() == 0);
    ui.key(keys::DOWN);
    CHECK(menu->highlighted() == 2);
    ui.key(keys::ENTER);
    CHECK(snap);
    CHECK_FALSE(ui.app.hasPopups());

    menu = builder.build();
    ui.app.pushPopup(menu, Rect::fromSize({20.0f, 20.0f}, glm::vec2(0.0f)), options);
    ui.frame();
    const glm::vec2 insert = ui.rectOf(menu->row(3)).center();
    ui.move(insert.x, insert.y);
    ui.frame();
    CHECK_FALSE(menu->subMenu());
    ui.platform.advance(Menu::SUBMENU_DELAY + 0.05);
    ui.frame();
    REQUIRE(menu->subMenu());
    CHECK(ui.app.popupCount() == 2);
    ui.frame();
    const Rect parent = ui.rectOf(menu->row(3));
    const Rect child = *ui.app.popupRect(menu->subMenu().get());
    CHECK(child.min.x >= parent.max.x);
    CHECK(ui.rectOf(menu->subMenu()->row(0)).min.y == parent.min.y);
    clickAt(ui, ui.rectOf(menu->subMenu()->row(1)).center());
    CHECK(inserted == "sphere");
    CHECK_FALSE(ui.app.hasPopups());

    menu = builder.build();
    ui.app.pushPopup(menu, Rect::fromSize({20.0f, 20.0f}, glm::vec2(0.0f)), options);
    ui.frame();
    ui.key(keys::UP);
    CHECK(menu->highlighted() == 3);
    ui.key(keys::RIGHT);
    REQUIRE(menu->subMenu());
    CHECK(ui.app.focused() == menu->subMenu());
    CHECK(menu->subMenu()->highlighted() == 0);
    ui.frame();
    ui.key(keys::LEFT);
    CHECK(ui.app.popupCount() == 1);
    CHECK(ui.app.focused() == menu);
    CHECK_FALSE(menu->subMenu());
}

TEST_CASE("a menu entry bound to a command shows its shortcut and runs through the command list") {
    auto command = std::make_shared<Command>(Command{"save", "Save", "Save the scene", Shortcut::primary(keys::letter('s'))});
    auto commands = std::make_shared<CommandList>();
    int runs = 0;
    bool allowed = true;
    commands->map(command, CommandAction{[&] { ++runs; }, [&] { return allowed; }, {}});
    MenuBuilder builder;
    builder.command(commands, *command);
    REQUIRE(builder.items().size() == 1);
    CHECK(builder.items()[0].shortcut == command->shortcut.label());
    CHECK(builder.items()[0].label.get() == "Save");
    builder.items()[0].action();
    CHECK(runs == 1);
    allowed = false;
    CHECK_FALSE(builder.items()[0].canExecute());
}

TEST_CASE("a menu bar opens on press, follows the pointer across titles, and a press-drag-release picks") {
    int saved = 0;
    std::shared_ptr<MenuBar> bar;
    Harness ui(at({0.0f, 0.0f}, {400.0f, 24.0f},
                  make<MenuBar>()
                          .assign(bar)
                          .menu("File", [&](MenuBuilder& menu) { menu.entry("Save", [&] { ++saved; }); })
                          .menu("Edit", [&](MenuBuilder& menu) { menu.entry("Undo", [] {}); })));
    const glm::vec2 file = ui.rectOf(bar->item(0)).center();
    const glm::vec2 edit = ui.rectOf(bar->item(1)).center();
    ui.move(file.x, file.y);
    ui.press();
    CHECK(bar->openIndex() == 0);
    ui.frame();
    const auto menu = std::dynamic_pointer_cast<Menu>(ui.app.focused());
    REQUIRE(menu);
    const glm::vec2 save = ui.rectOf(menu->row(0)).center();
    ui.move(save.x, save.y);
    ui.release();
    CHECK(saved == 1);
    CHECK_FALSE(ui.app.hasPopups());

    ui.frame();
    ui.platform.advance(1.0);
    ui.click(file.x, file.y);
    ui.frame();
    ui.move(edit.x, edit.y);
    CHECK(bar->openIndex() == 1);
    ui.frame();
    ui.platform.advance(1.0);
    ui.click(edit.x, edit.y);
    CHECK(bar->openIndex() == -1);
    CHECK_FALSE(ui.app.hasPopups());
}

TEST_CASE("an expandable area folds its body away from its header") {
    std::shared_ptr<ExpandableArea> area;
    std::shared_ptr<Probe> inner;
    bool expanded = true;
    Harness ui(make<ExpandableArea>()
                       .assign(area)
                       .areaTitle("Transform")
                       .bodyContent(make<Probe>().assign(inner))
                       .onAreaExpansionChanged([&](bool open) { expanded = open; }));
    CHECK(ui.painted(inner));
    clickAt(ui, ui.rectOf(area->header()).center());
    CHECK_FALSE(expanded);
    ui.frame();
    CHECK_FALSE(ui.painted(inner));
    CHECK(area->desiredSize().y < 40.0f);
    ui.platform.advance(1.0);
    clickAt(ui, ui.rectOf(area->header()).center());
    ui.frame();
    CHECK(ui.painted(inner));
}

TEST_CASE("HSV and hex conversions round trip") {
    CHECK(rgbToHsv({1.0f, 0.0f, 0.0f}) == glm::vec3(0.0f, 1.0f, 1.0f));
    const glm::vec3 green = hsvToRgb({1.0f / 3.0f, 1.0f, 1.0f});
    CHECK(green.g == doctest::Approx(1.0f));
    CHECK(green.r == doctest::Approx(0.0f).epsilon(1e-5));
    for (const glm::vec3 rgb : {glm::vec3(0.2f, 0.4f, 0.9f), glm::vec3(0.9f, 0.8f, 0.1f), glm::vec3(0.5f, 0.1f, 0.6f)}) {
        const glm::vec3 back = hsvToRgb(rgbToHsv(rgb));
        CHECK(glm::distance(back, rgb) < 1e-5f);
    }
    CHECK(toHex(Color::hex(0x3366CCFF), false) == "3366CC");
    CHECK(toHex(Color::hex(0x3366CC80), true) == "3366CC80");
    CHECK(fromHex("#3366cc") == Color::hex(0x3366CCFF));
    CHECK(fromHex("3366CC80") == Color::hex(0x3366CC80));
    CHECK_FALSE(fromHex("zz0000"));
    CHECK_FALSE(fromHex("123"));
}

TEST_CASE("a colour block opens a picker whose square sets saturation and value, keeping the hue through grey") {
    Color value{1.0f, 0.0f, 0.0f, 1.0f};
    std::shared_ptr<ColorBlock> block;
    Harness ui(at({10.0f, 10.0f}, {48.0f, 18.0f},
                  make<ColorBlock>()
                          .assign(block)
                          .color([&] { return value; })
                          .opensPicker(true)
                          .onColorChanged([&](Color next) { value = next; })));
    ui.size = {800.0f, 600.0f};
    ui.frame();
    ui.click(20.0f, 20.0f);
    REQUIRE(block->isPickerOpen());
    ui.frame();
    const Rect popup = *ui.app.popupRect(block->picker().get());
    CHECK(popup.min.y == 28.0f);

    const glm::vec2 square = popup.min + glm::vec2(10.0f);
    ui.move(square.x + ColorPicker::AREA * 0.5f, square.y + ColorPicker::AREA * 0.25f);
    ui.press();
    ui.release();
    CHECK(value.r == doctest::Approx(Color::toLinear(0.75f)).epsilon(0.01));
    CHECK(value.g == doctest::Approx(Color::toLinear(0.375f)).epsilon(0.01));
    CHECK(value.b == doctest::Approx(value.g));

    auto picker = std::dynamic_pointer_cast<ColorPicker>(block->picker());
    REQUIRE(picker);
    picker->setColor(Color{0.2f, 0.2f, 0.2f, 1.0f});
    CHECK(picker->hsv().x == doctest::Approx(0.0f));
    picker->setHsv({0.5f, 0.0f, 0.5f});
    picker->setColor(Color{0.0f, 0.0f, 0.0f, 1.0f});
    CHECK(picker->hsv().x == doctest::Approx(0.5f));

    ui.frame();
    ui.platform.advance(1.0);
    ui.click(20.0f, 20.0f);
    CHECK_FALSE(block->isPickerOpen());
}
