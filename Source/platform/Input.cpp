#include "platform/Input.hpp"

#include "platform/Log.hpp"
#include "platform/Window.hpp"

#include <GLFW/glfw3.h>

#include <cctype>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>

namespace cinder::platform {
namespace {

std::unordered_map<std::string, int> buildKeys() {
    std::unordered_map<std::string, int> keys;
    for (char c = 'a'; c <= 'z'; ++c) keys[std::string(1, c)] = GLFW_KEY_A + (c - 'a');
    for (char c = '0'; c <= '9'; ++c) keys[std::string(1, c)] = GLFW_KEY_0 + (c - '0');
    for (int i = 1; i <= 12; ++i) keys["f" + std::to_string(i)] = GLFW_KEY_F1 + (i - 1);

    keys["space"] = GLFW_KEY_SPACE;
    keys["escape"] = GLFW_KEY_ESCAPE;
    keys["enter"] = GLFW_KEY_ENTER;
    keys["tab"] = GLFW_KEY_TAB;
    keys["backspace"] = GLFW_KEY_BACKSPACE;
    keys["delete"] = GLFW_KEY_DELETE;
    keys["insert"] = GLFW_KEY_INSERT;
    keys["home"] = GLFW_KEY_HOME;
    keys["end"] = GLFW_KEY_END;
    keys["pageup"] = GLFW_KEY_PAGE_UP;
    keys["pagedown"] = GLFW_KEY_PAGE_DOWN;
    keys["left"] = GLFW_KEY_LEFT;
    keys["right"] = GLFW_KEY_RIGHT;
    keys["up"] = GLFW_KEY_UP;
    keys["down"] = GLFW_KEY_DOWN;
    keys["lshift"] = GLFW_KEY_LEFT_SHIFT;
    keys["rshift"] = GLFW_KEY_RIGHT_SHIFT;
    keys["lctrl"] = GLFW_KEY_LEFT_CONTROL;
    keys["rctrl"] = GLFW_KEY_RIGHT_CONTROL;
    keys["lalt"] = GLFW_KEY_LEFT_ALT;
    keys["ralt"] = GLFW_KEY_RIGHT_ALT;
    keys["lsuper"] = GLFW_KEY_LEFT_SUPER;
    keys["rsuper"] = GLFW_KEY_RIGHT_SUPER;
    keys["comma"] = GLFW_KEY_COMMA;
    keys["period"] = GLFW_KEY_PERIOD;
    keys["minus"] = GLFW_KEY_MINUS;
    keys["equal"] = GLFW_KEY_EQUAL;
    keys["slash"] = GLFW_KEY_SLASH;
    keys["semicolon"] = GLFW_KEY_SEMICOLON;
    keys["apostrophe"] = GLFW_KEY_APOSTROPHE;
    return keys;
}

const std::unordered_map<std::string, int>& keyNames() {
    static const std::unordered_map<std::string, int> keys = buildKeys();
    return keys;
}

const std::unordered_map<std::string, int>& buttonNames() {
    static const std::unordered_map<std::string, int> buttons = {
        {"left", GLFW_MOUSE_BUTTON_LEFT},
        {"right", GLFW_MOUSE_BUTTON_RIGHT},
        {"middle", GLFW_MOUSE_BUTTON_MIDDLE},
    };
    return buttons;
}

int lookup(const std::unordered_map<std::string, int>& table, std::string_view name, const char* kind) {
    std::string lowered(name);
    for (char& c : lowered) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    auto found = table.find(lowered);
    if (found != table.end()) return found->second;

    static std::set<std::string> warned;
    if (warned.insert(std::string(kind) + ":" + lowered).second) {
        logError("[input] unknown %s \"%s\"\n", kind, lowered.c_str());
    }
    return -1;
}

bool query(const bool* state, int count, int code) {
    return code >= 0 && code < count && state[code];
}

Input* target(GLFWwindow* handle) {
    Input* input = static_cast<Window*>(glfwGetWindowUserPointer(handle))->input();
    return input != nullptr && !input->ignoresSystem() ? input : nullptr;
}

bool releases(InputEventType type) {
    return type == InputEventType::KeyUp || type == InputEventType::MouseUp || type == InputEventType::FocusLost;
}

}

Input::Input() = default;

Input::Input(Window& window) : window_(&window) {
    window.attach(this);

    GLFWwindow* handle = window.handle();

    glfwSetKeyCallback(handle, [](GLFWwindow* w, int key, int, int action, int mods) {
        if (Input* input = target(w)) input->onKey(key, action, mods);
    });
    glfwSetCharCallback(handle, [](GLFWwindow* w, unsigned int codepoint) {
        if (Input* input = target(w)) input->onChar(static_cast<char32_t>(codepoint));
    });
    glfwSetMouseButtonCallback(handle, [](GLFWwindow* w, int button, int action, int mods) {
        if (Input* input = target(w)) input->onMouseButton(button, action, mods);
    });
    glfwSetCursorPosCallback(handle, [](GLFWwindow* w, double x, double y) {
        if (Input* input = target(w)) input->onCursorPos(x, y);
    });
    glfwSetScrollCallback(handle, [](GLFWwindow* w, double x, double y) {
        if (Input* input = target(w)) input->onScroll(x, y);
    });
    glfwSetCursorEnterCallback(handle, [](GLFWwindow* w, int entered) {
        if (Input* input = target(w)) input->onCursorEnter(entered == GLFW_TRUE);
    });
    glfwSetWindowFocusCallback(handle, [](GLFWwindow* w, int focused) {
        if (Input* input = target(w)) input->onFocus(focused == GLFW_TRUE);
    });
}

std::uint8_t Input::modifiers() const {
    std::uint8_t result = 0;
    if (held(keys::LEFT_SHIFT) || held(keys::RIGHT_SHIFT)) result |= modifiers::SHIFT;
    if (held(keys::LEFT_CONTROL) || held(keys::RIGHT_CONTROL)) result |= modifiers::CONTROL;
    if (held(keys::LEFT_ALT) || held(keys::RIGHT_ALT)) result |= modifiers::ALT;
    if (held(keys::LEFT_SUPER) || held(keys::RIGHT_SUPER)) result |= modifiers::SUPER;
    return result;
}

InputEvent Input::pointer(InputEventType type, int code, int mods) const {
    InputEvent event;
    event.type = type;
    event.code = code;
    event.modifiers = static_cast<std::uint8_t>(mods);
    event.position = cursor();
    return event;
}

void Input::queue(InputEvent event) {
    if (!recording_) return;
    if (event.type == InputEventType::MouseMove && !events_.empty()
            && events_.back().type == InputEventType::MouseMove) {
        events_.back() = event;
        return;
    }
    if (events_.size() >= MAX_EVENTS && !releases(event.type)) return;
    events_.push_back(event);
}

void Input::setRecording(bool recording) {
    recording_ = recording;
    if (!recording) events_.clear();
}

std::vector<InputEvent> Input::takeEvents() {
    std::vector<InputEvent> taken;
    taken.swap(events_);
    return taken;
}

void Input::onKey(int key, int action, int mods) {
    if (key < 0 || key >= keys::COUNT) return;
    const auto index = static_cast<std::size_t>(key);
    if (action == GLFW_PRESS) {
        keyDown_[index] = true;
        keyPressed_[index] = true;
        queue(pointer(InputEventType::KeyDown, key, mods));
    } else if (action == GLFW_REPEAT) {
        InputEvent event = pointer(InputEventType::KeyDown, key, mods);
        event.repeat = true;
        queue(event);
    } else if (action == GLFW_RELEASE) {
        keyDown_[index] = false;
        keyReleased_[index] = true;
        queue(pointer(InputEventType::KeyUp, key, mods));
    }
}

void Input::onChar(char32_t character) {
    InputEvent event = pointer(InputEventType::Char, 0, modifiers());
    event.character = character;
    queue(event);
}

void Input::onMouseButton(int button, int action, int mods) {
    if (button < 0 || button >= buttons::COUNT) return;
    const auto index = static_cast<std::size_t>(button);
    if (action == GLFW_PRESS) {
        buttonDown_[index] = true;
        buttonPressed_[index] = true;
        queue(pointer(InputEventType::MouseDown, button, mods));
    } else if (action == GLFW_RELEASE) {
        buttonDown_[index] = false;
        buttonReleased_[index] = true;
        queue(pointer(InputEventType::MouseUp, button, mods));
    }
}

void Input::onCursorPos(double x, double y) {
    if (firstCursorEvent_) {
        lastMouseX_ = x;
        lastMouseY_ = y;
        firstCursorEvent_ = false;
    }
    mouseX_ = x;
    mouseY_ = y;
    queue(pointer(InputEventType::MouseMove, 0, modifiers()));
}

void Input::onScroll(double x, double y) {
    scrollX_ += x;
    scrollY_ += y;
    InputEvent event = pointer(InputEventType::Wheel, 0, modifiers());
    event.wheel = glm::vec2(static_cast<float>(x), static_cast<float>(y));
    queue(event);
}

void Input::onCursorEnter(bool entered) {
    queue(pointer(entered ? InputEventType::CursorEnter : InputEventType::CursorLeave, 0, modifiers()));
}

void Input::onFocus(bool focused) {
    if (focused) return;
    for (int key = 0; key < keys::COUNT; ++key) {
        if (held(key)) onKey(key, GLFW_RELEASE, 0);
    }
    for (int button = 0; button < buttons::COUNT; ++button) {
        if (buttonHeld(button)) onMouseButton(button, GLFW_RELEASE, 0);
    }
    queue(pointer(InputEventType::FocusLost, 0, 0));
}

void Input::inject(const InputEvent& event) {
    const int mods = event.modifiers != 0 ? event.modifiers : modifiers();
    switch (event.type) {
        case InputEventType::KeyDown: onKey(event.code, event.repeat ? GLFW_REPEAT : GLFW_PRESS, mods); break;
        case InputEventType::KeyUp: onKey(event.code, GLFW_RELEASE, mods); break;
        case InputEventType::Char: onChar(event.character); break;
        case InputEventType::MouseDown: onMouseButton(event.code, GLFW_PRESS, mods); break;
        case InputEventType::MouseUp: onMouseButton(event.code, GLFW_RELEASE, mods); break;
        case InputEventType::MouseMove: onCursorPos(event.position.x, event.position.y); break;
        case InputEventType::Wheel: onScroll(event.wheel.x, event.wheel.y); break;
        case InputEventType::CursorEnter: onCursorEnter(true); break;
        case InputEventType::CursorLeave: onCursorEnter(false); break;
        case InputEventType::FocusLost: onFocus(false); break;
    }
}

int Input::keyCode(std::string_view name) { return lookup(keyNames(), name, "key"); }

int Input::buttonCode(std::string_view name) { return lookup(buttonNames(), name, "mouse button"); }

bool Input::keyDown(std::string_view name) const {
    if (keyboardSuppressed_) return false;
    return query(keyDown_.data(), keys::COUNT, keyCode(name));
}

bool Input::keyPressed(std::string_view name) const {
    if (keyboardSuppressed_) return false;
    return query(keyPressed_.data(), keys::COUNT, keyCode(name));
}

bool Input::keyReleased(std::string_view name) const {
    if (keyboardSuppressed_) return false;
    return query(keyReleased_.data(), keys::COUNT, keyCode(name));
}

bool Input::mouseDown(std::string_view name) const {
    if (mouseSuppressed_) return false;
    return query(buttonDown_.data(), buttons::COUNT, buttonCode(name));
}

bool Input::mousePressed(std::string_view name) const {
    if (mouseSuppressed_) return false;
    return query(buttonPressed_.data(), buttons::COUNT, buttonCode(name));
}

bool Input::mouseReleased(std::string_view name) const {
    if (mouseSuppressed_) return false;
    return query(buttonReleased_.data(), buttons::COUNT, buttonCode(name));
}

void Input::setSuppressed(bool keyboard, bool mouse) {
    keyboardSuppressed_ = keyboard;
    mouseSuppressed_ = mouse;
}

void Input::setViewportOrigin(double x, double y) {
    originX_ = x;
    originY_ = y;
}

void Input::setCursorLocked(bool locked) {
    if (locked == cursorLocked_) return;
    cursorLocked_ = locked;
    firstCursorEvent_ = true;
    if (window_ == nullptr) return;

    GLFWwindow* handle = window_->handle();
    glfwSetInputMode(handle, GLFW_CURSOR, locked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    if (glfwRawMouseMotionSupported()) {
        glfwSetInputMode(handle, GLFW_RAW_MOUSE_MOTION, locked ? GLFW_TRUE : GLFW_FALSE);
    }
}

void Input::consume() {
    keyPressed_.fill(false);
    keyReleased_.fill(false);
    buttonPressed_.fill(false);
    buttonReleased_.fill(false);
    lastMouseX_ = mouseX_;
    lastMouseY_ = mouseY_;
    scrollX_ = 0;
    scrollY_ = 0;
}

}
