#pragma once

#include <string>

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

    void attach(Input* input) { input_ = input; }
    Input* input() const { return input_; }

private:
    GLFWwindow* handle_ = nullptr;
    Input* input_ = nullptr;
    int width_ = 0;
    int height_ = 0;
    int logicalWidth_ = 0;
    int logicalHeight_ = 0;
    bool resized_ = false;
};

}
