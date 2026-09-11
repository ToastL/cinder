#pragma once

#include <array>
#include <string_view>

namespace cinder::platform {

class Window;

class Input {
public:
    explicit Input(Window& window);

    Input(const Input&) = delete;
    Input& operator=(const Input&) = delete;

    bool keyDown(std::string_view name) const;
    bool keyPressed(std::string_view name) const;
    bool keyReleased(std::string_view name) const;

    bool mouseDown(std::string_view name) const;
    bool mousePressed(std::string_view name) const;
    bool mouseReleased(std::string_view name) const;

    double mouseX() const { return mouseX_; }
    double mouseY() const { return mouseY_; }
    double mouseDeltaX() const { return mouseX_ - lastMouseX_; }
    double mouseDeltaY() const { return mouseY_ - lastMouseY_; }
    double scrollX() const { return mouseSuppressed_ ? 0.0 : scrollX_; }
    double scrollY() const { return mouseSuppressed_ ? 0.0 : scrollY_; }

    void setCursorLocked(bool locked);
    bool cursorLocked() const { return cursorLocked_; }

    void setSuppressed(bool keyboard, bool mouse);

    void consume();

    void onKey(int key, int action);
    void onMouseButton(int button, int action);
    void onCursorPos(double x, double y);
    void onScroll(double x, double y);

private:
    static constexpr int KEY_COUNT = 349;
    static constexpr int BUTTON_COUNT = 8;

    Window& window_;

    std::array<bool, KEY_COUNT> keyDown_{};
    std::array<bool, KEY_COUNT> keyPressed_{};
    std::array<bool, KEY_COUNT> keyReleased_{};

    std::array<bool, BUTTON_COUNT> buttonDown_{};
    std::array<bool, BUTTON_COUNT> buttonPressed_{};
    std::array<bool, BUTTON_COUNT> buttonReleased_{};

    double mouseX_ = 0;
    double mouseY_ = 0;
    double lastMouseX_ = 0;
    double lastMouseY_ = 0;
    double scrollX_ = 0;
    double scrollY_ = 0;

    bool cursorLocked_ = false;
    bool firstCursorEvent_ = true;
    bool keyboardSuppressed_ = false;
    bool mouseSuppressed_ = false;
};

}
