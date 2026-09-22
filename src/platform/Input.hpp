#pragma once

#include "platform/InputEvent.hpp"

#include <glm/vec2.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace cinder::platform {

class Window;

class Input {
public:
    static constexpr std::size_t MAX_EVENTS = 4096;

    Input();
    explicit Input(Window& window);

    Input(const Input&) = delete;
    Input& operator=(const Input&) = delete;

    bool keyDown(std::string_view name) const;
    bool keyPressed(std::string_view name) const;
    bool keyReleased(std::string_view name) const;

    bool mouseDown(std::string_view name) const;
    bool mousePressed(std::string_view name) const;
    bool mouseReleased(std::string_view name) const;

    double mouseX() const { return mouseX_ - originX_; }
    double mouseY() const { return mouseY_ - originY_; }
    double mouseDeltaX() const { return mouseX_ - lastMouseX_; }
    double mouseDeltaY() const { return mouseY_ - lastMouseY_; }
    double scrollX() const { return mouseSuppressed_ ? 0.0 : scrollX_; }
    double scrollY() const { return mouseSuppressed_ ? 0.0 : scrollY_; }

    glm::vec2 cursor() const { return {static_cast<float>(mouseX_), static_cast<float>(mouseY_)}; }
    bool held(int key) const { return key >= 0 && key < keys::COUNT && keyDown_[static_cast<std::size_t>(key)]; }
    bool buttonHeld(int button) const {
        return button >= 0 && button < buttons::COUNT && buttonDown_[static_cast<std::size_t>(button)];
    }
    std::uint8_t modifiers() const;

    void setCursorLocked(bool locked);
    bool cursorLocked() const { return cursorLocked_; }

    void setSuppressed(bool keyboard, bool mouse);
    void setViewportOrigin(double x, double y);

    void consume();

    void setRecording(bool recording);
    bool recording() const { return recording_; }
    std::vector<InputEvent> takeEvents();

    void setIgnoreSystem(bool ignore) { ignoreSystem_ = ignore; }
    bool ignoresSystem() const { return ignoreSystem_; }
    void inject(const InputEvent& event);

    static int keyCode(std::string_view name);
    static int buttonCode(std::string_view name);

    void onKey(int key, int action, int mods = 0);
    void onChar(char32_t character);
    void onMouseButton(int button, int action, int mods = 0);
    void onCursorPos(double x, double y);
    void onScroll(double x, double y);
    void onCursorEnter(bool entered);
    void onFocus(bool focused);

private:
    void queue(InputEvent event);
    InputEvent pointer(InputEventType type, int code, int mods) const;

    Window* window_ = nullptr;

    std::array<bool, keys::COUNT> keyDown_{};
    std::array<bool, keys::COUNT> keyPressed_{};
    std::array<bool, keys::COUNT> keyReleased_{};

    std::array<bool, buttons::COUNT> buttonDown_{};
    std::array<bool, buttons::COUNT> buttonPressed_{};
    std::array<bool, buttons::COUNT> buttonReleased_{};

    double mouseX_ = 0;
    double mouseY_ = 0;
    double lastMouseX_ = 0;
    double lastMouseY_ = 0;
    double originX_ = 0;
    double originY_ = 0;
    double scrollX_ = 0;
    double scrollY_ = 0;

    std::vector<InputEvent> events_;

    bool cursorLocked_ = false;
    bool firstCursorEvent_ = true;
    bool keyboardSuppressed_ = false;
    bool mouseSuppressed_ = false;
    bool recording_ = false;
    bool ignoreSystem_ = false;
};

}
