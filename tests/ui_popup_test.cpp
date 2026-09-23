#include <doctest/doctest.h>

#include "ui_harness.hpp"

#include "ui/widgets/BoxPanel.hpp"
#include "ui/widgets/Button.hpp"
#include "ui/widgets/Canvas.hpp"

using namespace cinder::ui;
using uitest::Harness;
using uitest::Probe;
using uitest::count;
namespace keys = cinder::platform::keys;

namespace {

std::shared_ptr<Widget> at(glm::vec2 position, glm::vec2 size, std::shared_ptr<Widget> content) {
    return make<Canvas>() + Canvas::slot().offset(Margin(position.x, position.y, size.x, size.y))[std::move(content)];
}

}

TEST_CASE("a popup opens below its anchor and takes the clicks inside it") {
    std::shared_ptr<Probe> below;
    Harness ui(make<Probe>().assign(below));
    std::shared_ptr<Probe> content = make<Probe>().size(glm::vec2(80.0f, 40.0f));
    ui.app.pushPopup(content, Rect{{10.0f, 10.0f}, {60.0f, 30.0f}});
    ui.frame();
    CHECK(ui.rectOf(content) == Rect{{10.0f, 30.0f}, {90.0f, 70.0f}});
    ui.click(20.0f, 40.0f);
    CHECK(count(content->log, "down") == 1);
    CHECK(count(below->log, "down") == 0);
    CHECK(ui.app.hasPopups());
}

TEST_CASE("a click outside every popup closes them all and never reaches what is below") {
    std::shared_ptr<Probe> below;
    Harness ui(make<Probe>().assign(below));
    int dismissed = 0;
    PopupOptions options;
    options.onDismissed = [&] { ++dismissed; };
    ui.app.pushPopup(make<Probe>().size(glm::vec2(80.0f, 40.0f)), Rect{{10.0f, 10.0f}, {60.0f, 30.0f}}, options);
    ui.frame();
    ui.click(300.0f, 200.0f);
    CHECK(count(below->log, "down") == 0);
    CHECK(dismissed == 1);
    CHECK_FALSE(ui.app.hasPopups());

    ui.frame();
    ui.platform.advance(1.0);
    ui.click(300.0f, 200.0f);
    CHECK(count(below->log, "down") == 1);
}

TEST_CASE("a popup that does not fit below opens above, and stays inside the window") {
    Harness ui(make<Probe>());
    std::shared_ptr<Probe> content = make<Probe>().size(glm::vec2(80.0f, 40.0f));
    ui.app.pushPopup(content, Rect{{380.0f, 280.0f}, {395.0f, 295.0f}});
    ui.frame();
    CHECK(ui.rectOf(content) == Rect{{320.0f, 240.0f}, {400.0f, 280.0f}});

    std::shared_ptr<Probe> side = make<Probe>().size(glm::vec2(50.0f, 20.0f));
    PopupOptions right;
    right.placement = Placement::Right;
    ui.app.pushPopup(side, Rect{{370.0f, 50.0f}, {390.0f, 60.0f}}, right);
    ui.frame();
    CHECK(ui.rectOf(side).min == glm::vec2(320.0f, 50.0f));
}

TEST_CASE("clicking in a lower popup closes the ones above it") {
    Harness ui(make<Probe>());
    std::shared_ptr<Probe> first = make<Probe>().size(glm::vec2(80.0f, 40.0f));
    std::shared_ptr<Probe> second = make<Probe>().size(glm::vec2(80.0f, 40.0f));
    ui.app.pushPopup(first, Rect{{10.0f, 10.0f}, {20.0f, 20.0f}});
    PopupOptions right;
    right.placement = Placement::Right;
    ui.app.pushPopup(second, Rect{{200.0f, 10.0f}, {210.0f, 20.0f}}, right);
    ui.frame();
    CHECK(ui.app.popupCount() == 2);
    ui.click(30.0f, 30.0f);
    CHECK(count(first->log, "down") == 1);
    CHECK(ui.app.popupCount() == 1);
    CHECK(ui.app.isPopupOpen(first.get()));
}

TEST_CASE("a press on a popup's owner reaches the owner and leaves the popup open") {
    std::shared_ptr<Probe> owner;
    Harness ui(at({10.0f, 10.0f}, {60.0f, 20.0f}, make<Probe>().assign(owner)));
    PopupOptions options;
    options.owner = owner;
    ui.app.pushPopup(make<Probe>().size(glm::vec2(80.0f, 40.0f)), Rect{{10.0f, 10.0f}, {70.0f, 30.0f}}, options);
    ui.frame();
    ui.click(40.0f, 20.0f);
    CHECK(count(owner->log, "down") == 1);
    CHECK(ui.app.hasPopups());
}

TEST_CASE("Escape closes the top popup when nothing in it wants the key, and focus goes back") {
    std::shared_ptr<Probe> field;
    Harness ui(at({0.0f, 0.0f}, {100.0f, 30.0f}, make<Probe>().assign(field).focusable(true)));
    ui.click(50.0f, 15.0f);
    CHECK(ui.app.focused() == field);

    std::shared_ptr<Probe> popup = make<Probe>().size(glm::vec2(80.0f, 40.0f)).focusable(true).handles(false);
    ui.app.pushPopup(popup, Rect{{0.0f, 0.0f}, {100.0f, 30.0f}});
    CHECK(ui.app.focused() == popup);
    ui.frame();
    ui.key(keys::ESCAPE);
    CHECK(count(popup->log, "key") == 1);
    CHECK_FALSE(ui.app.hasPopups());
    CHECK(ui.app.focused() == field);
}

TEST_CASE("losing the window closes popups") {
    Harness ui(make<Probe>());
    ui.app.pushPopup(make<Probe>(), Rect{});
    uitest::InputEvent lost;
    lost.type = uitest::InputEventType::FocusLost;
    ui.send(lost);
    CHECK_FALSE(ui.app.hasPopups());
}

TEST_CASE("a tooltip shows after the hover delay, on a disabled widget too, and a press hides it until the pointer leaves") {
    Harness ui(make<HorizontalBox>()
               + HorizontalBox::slot().autoWidth()[make<Button>().text("Step").isEnabled(false).toolTipText("Only while paused")]
               + HorizontalBox::slot().fill(1.0f)[make<Probe>()]);
    ui.move(10.0f, 10.0f);
    ui.frame();
    CHECK(ui.app.visibleToolTip().empty());
    ui.platform.advance(Application::TOOLTIP_DELAY + 0.1);
    ui.frame();
    CHECK(ui.app.visibleToolTip() == "Only while paused");

    ui.press();
    ui.frame();
    CHECK(ui.app.visibleToolTip().empty());
    ui.release();
    ui.platform.advance(1.0);
    ui.frame();
    CHECK(ui.app.visibleToolTip().empty());

    ui.move(300.0f, 10.0f);
    ui.frame();
    ui.move(10.0f, 10.0f);
    ui.frame();
    ui.platform.advance(Application::TOOLTIP_DELAY + 0.1);
    ui.frame();
    CHECK(ui.app.visibleToolTip() == "Only while paused");
}

TEST_CASE("the tooltip is drawn above everything and is never hit") {
    std::shared_ptr<Probe> probe;
    Harness ui(make<Probe>().assign(probe).toolTipText("Hello"));
    ui.move(10.0f, 10.0f);
    ui.frame();
    ui.platform.advance(1.0);
    ui.frame();
    REQUIRE(ui.app.visibleToolTip() == "Hello");
    int highest = 0;
    bool text = false;
    for (const Element& element : ui.list.elements()) {
        highest = std::max(highest, element.layer);
        if (std::holds_alternative<TextElement>(element.shape)) text = true;
    }
    CHECK(text);
    CHECK(highest > 0);
    const WidgetPath path = ui.app.grid().pathAt({25.0f, 32.0f});
    REQUIRE_FALSE(path.empty());
    CHECK(path.back().widget == probe);
}

TEST_CASE("keys typed in the same batch as the click that focused a field still reach it") {
    std::shared_ptr<Probe> late = make<Probe>().focusable(true);
    Harness ui(make<Probe>());
    ui.app.setFocus(late);
    ui.key(keys::letter('a'));
    CHECK(count(late->log, "key") == 1);
    CHECK(ui.app.focused() == late);
    ui.frame();
    ui.key(keys::letter('a'));
    CHECK_FALSE(ui.app.focused());
}
