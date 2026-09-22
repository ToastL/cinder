#include <doctest/doctest.h>

#include "platform/Input.hpp"
#include "platform/InputEvent.hpp"
#include "platform/InputScript.hpp"

#include <GLFW/glfw3.h>

#include <vector>

using cinder::platform::Input;
using cinder::platform::InputEvent;
using cinder::platform::InputEventType;
using cinder::platform::InputScript;
namespace keys = cinder::platform::keys;
namespace modifiers = cinder::platform::modifiers;

TEST_CASE("the event queue stays empty until something records it") {
    Input input;
    input.onKey(GLFW_KEY_A, GLFW_PRESS, 0);
    input.onCursorPos(10.0, 20.0);
    CHECK(input.takeEvents().empty());
    CHECK(input.keyDown("a"));
}

TEST_CASE("keys queue in order with their modifiers, and a repeat reaches only the queue") {
    Input input;
    input.setRecording(true);
    input.onKey(GLFW_KEY_S, GLFW_PRESS, GLFW_MOD_SUPER);
    input.consume();
    input.onKey(GLFW_KEY_S, GLFW_REPEAT, GLFW_MOD_SUPER);
    CHECK_FALSE(input.keyPressed("s"));
    input.onChar(U'é');
    input.onKey(GLFW_KEY_S, GLFW_RELEASE, 0);

    const std::vector<InputEvent> events = input.takeEvents();
    REQUIRE(events.size() == 4);
    CHECK(events[0].type == InputEventType::KeyDown);
    CHECK(events[0].code == keys::letter('s'));
    CHECK(events[0].modifiers == modifiers::SUPER);
    CHECK_FALSE(events[0].repeat);
    CHECK(events[1].repeat);
    CHECK(events[2].type == InputEventType::Char);
    CHECK(events[2].character == U'é');
    CHECK(events[3].type == InputEventType::KeyUp);
    CHECK(input.takeEvents().empty());
}

TEST_CASE("consuming a fixed step clears game edges but never the queue") {
    Input input;
    input.setRecording(true);
    input.onMouseButton(GLFW_MOUSE_BUTTON_LEFT, GLFW_PRESS, 0);
    input.consume();
    CHECK_FALSE(input.mousePressed("left"));
    CHECK(input.mouseDown("left"));
    CHECK(input.takeEvents().size() == 1);
}

TEST_CASE("cursor moves coalesce into the latest position") {
    Input input;
    input.setRecording(true);
    input.onCursorPos(1.0, 2.0);
    input.onCursorPos(3.0, 4.0);
    input.onScroll(0.0, 1.0);
    input.onCursorPos(5.0, 6.0);

    const std::vector<InputEvent> events = input.takeEvents();
    REQUIRE(events.size() == 3);
    CHECK(events[0].position == glm::vec2(3.0f, 4.0f));
    CHECK(events[1].type == InputEventType::Wheel);
    CHECK(events[1].wheel == glm::vec2(0.0f, 1.0f));
    CHECK(events[2].position == glm::vec2(5.0f, 6.0f));
}

TEST_CASE("losing window focus releases everything held, for the game and the queue") {
    Input input;
    input.setRecording(true);
    input.onKey(GLFW_KEY_W, GLFW_PRESS, 0);
    input.onMouseButton(GLFW_MOUSE_BUTTON_RIGHT, GLFW_PRESS, 0);
    input.consume();
    input.takeEvents();

    input.onFocus(false);
    CHECK_FALSE(input.keyDown("w"));
    CHECK(input.keyReleased("w"));
    CHECK_FALSE(input.mouseDown("right"));

    const std::vector<InputEvent> events = input.takeEvents();
    REQUIRE(events.size() == 3);
    CHECK(events[0].type == InputEventType::KeyUp);
    CHECK(events[1].type == InputEventType::MouseUp);
    CHECK(events[2].type == InputEventType::FocusLost);
}

TEST_CASE("a full queue drops new input but keeps every release") {
    Input input;
    input.setRecording(true);
    for (std::size_t i = 0; i < Input::MAX_EVENTS + 10; ++i) input.onChar(U'x');
    input.onKey(GLFW_KEY_A, GLFW_RELEASE, 0);
    const std::vector<InputEvent> events = input.takeEvents();
    CHECK(events.size() == Input::MAX_EVENTS + 1);
    CHECK(events.back().type == InputEventType::KeyUp);
}

TEST_CASE("injected keys carry the modifiers held when they arrive") {
    Input input;
    input.setRecording(true);
    InputEvent command;
    command.type = InputEventType::KeyDown;
    command.code = keys::LEFT_SUPER;
    input.inject(command);
    InputEvent save;
    save.type = InputEventType::KeyDown;
    save.code = keys::letter('s');
    input.inject(save);

    const std::vector<InputEvent> events = input.takeEvents();
    REQUIRE(events.size() == 2);
    CHECK(events[1].modifiers == modifiers::SUPER);
    CHECK(input.held(keys::letter('s')));
}

TEST_CASE("an input script plays its steps on their frames") {
    const InputScript script = InputScript::parse(
            "# comment\n"
            "3 move 100 50.5\n"
            "3 click left\n"
            "5 press lsuper\n"
            "5 tap s\n"
            "6 type h\xC3\xA9\n"
            "7 scroll 0 -2\n"
            "8 capture out.png\n"
            "9 quit\n");

    Input input;
    input.setRecording(true);
    input.setIgnoreSystem(true);
    for (int frame = 0; frame <= 9; ++frame) script.apply(input, frame);
    const std::vector<InputEvent> events = input.takeEvents();
    REQUIRE(events.size() == 9);
    CHECK(events[0].position == glm::vec2(100.0f, 50.5f));
    CHECK(events[1].type == InputEventType::MouseDown);
    CHECK(events[1].position == glm::vec2(100.0f, 50.5f));
    CHECK(events[2].type == InputEventType::MouseUp);
    CHECK(events[4].modifiers == modifiers::SUPER);
    CHECK(events[6].character == U'h');
    CHECK(events[7].character == U'é');
    CHECK(events[8].wheel == glm::vec2(0.0f, -2.0f));
    CHECK(script.capture(8) == "out.png");
    CHECK_FALSE(script.capture(7));
    CHECK(script.quits(9));
    CHECK_FALSE(script.quits(8));
    CHECK(script.lastFrame() == 9);
}

TEST_CASE("a bad input script names the line") {
    CHECK_THROWS_WITH_AS(InputScript::parse("1 move 1 1\n2 hover 3\n"), doctest::Contains("line 2"), std::runtime_error);
    CHECK_THROWS_WITH_AS(InputScript::parse("x move 1 1\n"), doctest::Contains("line 1"), std::runtime_error);
}
