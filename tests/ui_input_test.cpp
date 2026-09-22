#include <doctest/doctest.h>

#include "ui_harness.hpp"

#include "ui/framework/Commands.hpp"
#include "ui/widgets/Border.hpp"
#include "ui/widgets/BoxPanel.hpp"
#include "ui/widgets/Button.hpp"
#include "ui/widgets/Canvas.hpp"
#include "ui/widgets/Splitter.hpp"
#include "ui/widgets/TextField.hpp"

using namespace cinder::ui;
using cinder::platform::CursorShape;
using uitest::Harness;
using uitest::Probe;
using uitest::count;
namespace keys = cinder::platform::keys;
namespace modifiers = cinder::platform::modifiers;

namespace {

std::shared_ptr<Widget> at(glm::vec2 position, glm::vec2 size, std::shared_ptr<Widget> content) {
    return make<Canvas>() + Canvas::slot().offset(Margin(position.x, position.y, size.x, size.y))[std::move(content)];
}

}

TEST_CASE("the topmost overlay slot is hit, and a pass-through one is not") {
    std::shared_ptr<Probe> below;
    std::shared_ptr<Probe> above;
    Harness ui(make<Overlay>() + Overlay::slot()[make<Probe>().assign(below)]
               + Overlay::slot()[make<Probe>().assign(above)]);
    ui.click(50.0f, 50.0f);
    CHECK(count(above->log, "down") == 1);
    CHECK(count(below->log, "down") == 0);

    above->setVisibility(Visibility::NoHitTest);
    ui.frame();
    ui.platform.advance(1.0);
    ui.click(50.0f, 50.0f);
    CHECK(count(below->log, "down") == 1);
}

TEST_CASE("a widget that lets its children be hit is itself skipped") {
    std::shared_ptr<Border> border;
    std::shared_ptr<Probe> inner;
    Harness ui(make<Border>().assign(border).visibility(Visibility::NoHitTestSelf).padding(Margin(50.0f))
                       [make<Probe>().assign(inner)]);
    CHECK(ui.app.grid().pathAt({10.0f, 10.0f}).empty());
    const WidgetPath path = ui.app.grid().pathAt({100.0f, 100.0f});
    REQUIRE(path.size() == 2);
    CHECK(path.back().widget == inner);
}

TEST_CASE("an unhandled press bubbles to the nearest ancestor that handles it") {
    std::shared_ptr<Probe> leaf;
    int bordered = 0;
    Harness ui(make<Border>().onMouseDown([&](const Geometry&, const PointerEvent&) {
        ++bordered;
        return Reply::handled();
    })[make<Probe>().assign(leaf).handles(false)]);
    ui.click(100.0f, 100.0f);
    CHECK(count(leaf->log, "down") == 1);
    CHECK(bordered == 1);
}

TEST_CASE("a clipped-away part of a widget is not hit") {
    std::shared_ptr<Probe> inner;
    Harness ui(make<Border>().clip(true).padding(Margin(0.0f))
                       [at({0.0f, 0.0f}, {1000.0f, 1000.0f}, make<Probe>().assign(inner))]);
    ui.size = {100.0f, 100.0f};
    ui.frame();
    CHECK_FALSE(ui.app.grid().pathAt({50.0f, 50.0f}).empty());
    CHECK(ui.app.grid().pathAt({150.0f, 50.0f}).empty());
}

TEST_CASE("a button clicks on release inside, and not after the pointer leaves") {
    int clicks = 0;
    std::shared_ptr<Button> button;
    Harness ui(at({10.0f, 10.0f}, {100.0f, 30.0f}, make<Button>().assign(button).text("Play").onClicked([&] {
        ++clicks;
        return Reply::handled();
    })));

    ui.click(50.0f, 20.0f);
    CHECK(clicks == 1);
    CHECK_FALSE(ui.app.captor());

    ui.move(50.0f, 20.0f);
    ui.press();
    CHECK(button->isPressed());
    CHECK(ui.app.captor() == button);
    ui.move(300.0f, 200.0f);
    CHECK_FALSE(button->isHovered());
    ui.release();
    CHECK(clicks == 1);
    CHECK_FALSE(button->isPressed());
    CHECK_FALSE(ui.app.captor());
}

TEST_CASE("a disabled button never clicks") {
    int clicks = 0;
    Harness ui(at({0.0f, 0.0f}, {100.0f, 30.0f}, make<Button>().text("Stop").isEnabled(false).onClicked([&] {
        ++clicks;
        return Reply::handled();
    })));
    ui.click(50.0f, 15.0f);
    CHECK(clicks == 0);
}

TEST_CASE("hovering enters and leaves the whole path under the pointer") {
    std::shared_ptr<Probe> left;
    std::shared_ptr<Probe> right;
    Harness ui(make<HorizontalBox>() + HorizontalBox::slot()[make<Probe>().assign(left)]
               + HorizontalBox::slot()[make<Probe>().assign(right)]);
    ui.move(10.0f, 10.0f);
    CHECK(left->isHovered());
    CHECK(ui.app.root()->isHovered());
    ui.move(300.0f, 10.0f);
    CHECK_FALSE(left->isHovered());
    CHECK(right->isHovered());
    CHECK(count(left->log, "enter") == 1);
    CHECK(count(left->log, "leave") == 1);
}

TEST_CASE("a second press close in time and place is a double click") {
    std::shared_ptr<Probe> probe;
    Harness ui(make<Probe>().assign(probe));
    ui.click(10.0f, 10.0f);
    ui.platform.advance(0.2);
    ui.click(11.0f, 10.0f);
    CHECK(count(probe->log, "double") == 1);
    ui.platform.advance(0.2);
    ui.click(11.0f, 10.0f);
    CHECK(count(probe->log, "double") == 1);
    ui.platform.advance(1.0);
    ui.click(11.0f, 10.0f);
    CHECK(count(probe->log, "double") == 1);
    ui.platform.advance(0.1);
    ui.click(40.0f, 10.0f);
    CHECK(count(probe->log, "double") == 1);
}

TEST_CASE("a drag is detected only past the threshold") {
    std::shared_ptr<Probe> probe;
    Harness ui(make<Probe>().assign(probe));
    struct Dragger : Probe {
        Reply onMouseDown(const Geometry&, const PointerEvent& event) override {
            return Reply::handled().detectDrag(shared_from_this(), event.button);
        }
    };
    auto dragger = std::make_shared<Dragger>();
    ui.app.setRoot(dragger);
    ui.frame();
    ui.move(10.0f, 10.0f);
    ui.press();
    ui.move(10.0f + Application::DRAG_THRESHOLD - 1.0f, 10.0f);
    CHECK(count(dragger->log, "drag") == 0);
    ui.move(10.0f + Application::DRAG_THRESHOLD + 1.0f, 10.0f);
    CHECK(count(dragger->log, "drag") == 1);
    ui.move(40.0f, 10.0f);
    CHECK(count(dragger->log, "drag") == 1);
    ui.release();
}

TEST_CASE("a press focuses the deepest focusable widget and a press elsewhere clears it") {
    std::shared_ptr<Probe> focusable;
    std::shared_ptr<Probe> plain;
    Harness ui(make<HorizontalBox>() + HorizontalBox::slot()[make<Probe>().assign(focusable).focusable(true)]
               + HorizontalBox::slot()[make<Probe>().assign(plain)]);
    ui.click(10.0f, 10.0f);
    CHECK(ui.app.focused() == focusable);
    CHECK(focusable->hasFocus());
    ui.click(300.0f, 10.0f);
    CHECK_FALSE(ui.app.focused());
    CHECK(count(focusable->log, "blur") == 1);
}

TEST_CASE("keys go to the focused widget first, then to global commands") {
    std::shared_ptr<Probe> probe;
    Harness ui(make<Probe>().assign(probe).focusable(true).handles(false));

    int saves = 0;
    auto save = std::make_shared<Command>(Command{"Save", "Save", "", Shortcut::primary(keys::letter('s'))});
    auto commands = std::make_shared<CommandList>();
    commands->map(save, CommandAction{[&] { ++saves; }, {}, {}});
    ui.app.addCommands(commands);

    ui.key(keys::letter('s'), modifiers::PRIMARY);
    CHECK(saves == 1);
    ui.app.setFocus(probe);
    ui.key(keys::letter('s'), modifiers::PRIMARY);
    CHECK(saves == 2);
    ui.key(keys::letter('s'), modifiers::PRIMARY | modifiers::SHIFT);
    CHECK(count(probe->log, "key") == 2);
    CHECK(saves == 2);

    probe->handles_ = true;
    ui.key(keys::letter('s'), modifiers::PRIMARY);
    CHECK(saves == 2);
}

TEST_CASE("a text field keeps the primary undo chord for itself") {
    std::shared_ptr<TextField> field;
    Harness ui(at({0.0f, 0.0f}, {200.0f, 24.0f}, make<TextField>().assign(field).text("abc")));
    int undos = 0;
    auto undo = std::make_shared<Command>(Command{"Undo", "Undo", "", Shortcut::primary(keys::letter('z'))});
    auto commands = std::make_shared<CommandList>();
    commands->map(undo, CommandAction{[&] { ++undos; }, {}, {}});
    ui.app.addCommands(commands);

    ui.key(keys::letter('z'), modifiers::PRIMARY);
    CHECK(undos == 1);
    ui.click(190.0f, 12.0f);
    ui.type(U"d");
    CHECK(ui.app.isInteracting());
    ui.key(keys::letter('z'), modifiers::PRIMARY);
    CHECK(undos == 1);
    CHECK(field->text() == "abc");
}

TEST_CASE("disabled commands swallow their chord, and a shortcut prints its label") {
    int runs = 0;
    auto play = std::make_shared<Command>(Command{"Play", "Play", "", Shortcut::primary(keys::letter('p'))});
    CommandList commands;
    commands.map(play, CommandAction{[&] { ++runs; }, [] { return false; }, {}});
    KeyEvent event;
    event.key = keys::letter('p');
    event.modifiers = modifiers::PRIMARY;
    CHECK(commands.process(event));
    CHECK(runs == 0);
    CHECK_FALSE(commands.execute(*play));
#if defined(__APPLE__)
    CHECK(Shortcut::primary(keys::letter('p'), modifiers::SHIFT).label() == "\xE2\x87\xA7\xE2\x8C\x98P");
#else
    CHECK(Shortcut::primary(keys::letter('p'), modifiers::SHIFT).label() == "Ctrl+Shift+P");
#endif
}

TEST_CASE("the cursor follows what is under the pointer") {
    Harness ui(make<Splitter>() + Splitter::slot()[make<TextField>().text("x")] + Splitter::slot()[make<Probe>()]);
    ui.move(20.0f, 10.0f);
    CHECK(ui.platform.cursor() == CursorShape::IBeam);
    ui.move(200.0f, 10.0f);
    CHECK(ui.platform.cursor() == CursorShape::ResizeHorizontal);
    ui.move(300.0f, 10.0f);
    CHECK(ui.platform.cursor() == CursorShape::Arrow);
}

TEST_CASE("dragging a splitter handle moves the division and keeps a minimum") {
    std::shared_ptr<Splitter> splitter;
    Harness ui(make<Splitter>().assign(splitter) + Splitter::slot()[make<Probe>()] + Splitter::slot()[make<Probe>()]);
    ui.move(200.0f, 50.0f);
    ui.press();
    ui.move(100.0f, 50.0f);
    ui.release();
    CHECK(splitter->slotValue(0) / (splitter->slotValue(0) + splitter->slotValue(1)) == doctest::Approx(98.0f / 396.0f));

    ui.frame();
    ui.move(100.0f, 50.0f);
    ui.press();
    ui.move(-500.0f, 50.0f);
    ui.release();
    ui.frame();
    CHECK(splitter->slotValue(0) / (splitter->slotValue(0) + splitter->slotValue(1))
          == doctest::Approx(Splitter::MIN_SLOT / 396.0f));
}

TEST_CASE("losing the window drops capture and hover") {
    std::shared_ptr<Button> button;
    Harness ui(make<Button>().assign(button).text("x"));
    ui.move(10.0f, 10.0f);
    ui.press();
    CHECK(ui.app.captor() == button);
    uitest::InputEvent lost;
    lost.type = uitest::InputEventType::FocusLost;
    ui.send(lost);
    CHECK_FALSE(ui.app.captor());
    CHECK_FALSE(button->isHovered());
    CHECK_FALSE(button->isPressed());
}
