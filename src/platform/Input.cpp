#include "platform/Input.hpp"

#include "platform/Log.hpp"
#include "platform/Window.hpp"

#include <GLFW/glfw3.h>

#include <cctype>
#include <set>
#include <string>
#include <unordered_map>

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
    keys["comma"] = GLFW_KEY_COMMA;
    keys["period"] = GLFW_KEY_PERIOD;
    keys["minus"] = GLFW_KEY_MINUS;
    keys["equal"] = GLFW_KEY_EQUAL;
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

}

Input::Input(Window& window) : window_(window) {
    window.attach(this);

    GLFWwindow* handle = window.handle();

    glfwSetKeyCallback(handle, [](GLFWwindow* w, int key, int, int action, int) {
        static_cast<Window*>(glfwGetWindowUserPointer(w))->input()->onKey(key, action);
    });
    glfwSetMouseButtonCallback(handle, [](GLFWwindow* w, int button, int action, int) {
        static_cast<Window*>(glfwGetWindowUserPointer(w))->input()->onMouseButton(button, action);
    });
    glfwSetCursorPosCallback(handle, [](GLFWwindow* w, double x, double y) {
        static_cast<Window*>(glfwGetWindowUserPointer(w))->input()->onCursorPos(x, y);
    });
    glfwSetScrollCallback(handle, [](GLFWwindow* w, double x, double y) {
        static_cast<Window*>(glfwGetWindowUserPointer(w))->input()->onScroll(x, y);
    });
}

void Input::onKey(int key, int action) {
    if (key < 0 || key >= KEY_COUNT) return;
    if (action == GLFW_PRESS) {
        keyDown_[key] = true;
        keyPressed_[key] = true;
    } else if (action == GLFW_RELEASE) {
        keyDown_[key] = false;
        keyReleased_[key] = true;
    }
}

void Input::onMouseButton(int button, int action) {
    if (button < 0 || button >= BUTTON_COUNT) return;
    if (action == GLFW_PRESS) {
        buttonDown_[button] = true;
        buttonPressed_[button] = true;
    } else if (action == GLFW_RELEASE) {
        buttonDown_[button] = false;
        buttonReleased_[button] = true;
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
}

void Input::onScroll(double x, double y) {
    scrollX_ += x;
    scrollY_ += y;
}

bool Input::keyDown(std::string_view name) const {
    if (keyboardSuppressed_) return false;
    return query(keyDown_.data(), KEY_COUNT, lookup(keyNames(), name, "key"));
}

bool Input::keyPressed(std::string_view name) const {
    if (keyboardSuppressed_) return false;
    return query(keyPressed_.data(), KEY_COUNT, lookup(keyNames(), name, "key"));
}

bool Input::keyReleased(std::string_view name) const {
    if (keyboardSuppressed_) return false;
    return query(keyReleased_.data(), KEY_COUNT, lookup(keyNames(), name, "key"));
}

bool Input::mouseDown(std::string_view name) const {
    if (mouseSuppressed_) return false;
    return query(buttonDown_.data(), BUTTON_COUNT, lookup(buttonNames(), name, "mouse button"));
}

bool Input::mousePressed(std::string_view name) const {
    if (mouseSuppressed_) return false;
    return query(buttonPressed_.data(), BUTTON_COUNT, lookup(buttonNames(), name, "mouse button"));
}

bool Input::mouseReleased(std::string_view name) const {
    if (mouseSuppressed_) return false;
    return query(buttonReleased_.data(), BUTTON_COUNT, lookup(buttonNames(), name, "mouse button"));
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

    GLFWwindow* handle = window_.handle();
    glfwSetInputMode(handle, GLFW_CURSOR, locked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    if (glfwRawMouseMotionSupported()) {
        glfwSetInputMode(handle, GLFW_RAW_MOUSE_MOTION, locked ? GLFW_TRUE : GLFW_FALSE);
    }
    firstCursorEvent_ = true;
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
