#pragma once

#include "platform/Cursor.hpp"
#include "platform/Glfw.hpp"

#include <array>
#include <cstddef>
#include <string>
#include <string_view>

struct GLFWcursor;
struct GLFWwindow;

namespace cinder::platform {

class Input;

class Window {
public:
    Window(const std::string& title, int width, int height);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    GLFWwindow* handle() const { return handle_; }

    int width() const { return width_; }
    int height() const { return height_; }
    int logicalWidth() const { return logicalWidth_; }
    int logicalHeight() const { return logicalHeight_; }

    bool shouldClose() const;
    void setShouldClose(bool close);
    bool isMinimized() const { return width_ == 0 || height_ == 0; }
    bool wasResized() const { return resized_; }
    void clearResized() { resized_ = false; }

    std::string clipboard() const;
    void setClipboard(std::string_view text);
    void setCursor(CursorShape shape);
    float contentScale() const;

    void attach(Input* input) { input_ = input; }
    Input* input() const { return input_; }

private:
    GlfwSession glfw_;
    GLFWwindow* handle_ = nullptr;
    Input* input_ = nullptr;
    std::array<GLFWcursor*, static_cast<std::size_t>(CursorShape::Count)> cursors_{};
    CursorShape cursor_ = CursorShape::Arrow;
    int width_ = 0;
    int height_ = 0;
    int logicalWidth_ = 0;
    int logicalHeight_ = 0;
    bool resized_ = false;
};

}
