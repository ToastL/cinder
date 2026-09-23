#include "platform/Window.hpp"

#include "platform/Glfw.hpp"

#include <GLFW/glfw3.h>

#include <stdexcept>

namespace cinder::platform {

Window::Window(const std::string& title, int width, int height) {
    Glfw::acquire();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    handle_ = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (handle_ == nullptr) {
        Glfw::release();
        throw std::runtime_error("Failed to create window");
    }

    glfwGetFramebufferSize(handle_, &width_, &height_);
    glfwGetWindowSize(handle_, &logicalWidth_, &logicalHeight_);

    glfwSetWindowUserPointer(handle_, this);

    glfwSetFramebufferSizeCallback(handle_, [](GLFWwindow* window, int w, int h) {
        auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
        self->width_ = w;
        self->height_ = h;
        self->resized_ = true;
    });

    glfwSetWindowSizeCallback(handle_, [](GLFWwindow* window, int w, int h) {
        auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
        self->logicalWidth_ = w;
        self->logicalHeight_ = h;
    });
}

Window::~Window() {
    for (GLFWcursor* cursor : cursors_) {
        if (cursor != nullptr) glfwDestroyCursor(cursor);
    }
    if (handle_ != nullptr) glfwDestroyWindow(handle_);
    Glfw::release();
}

std::string Window::clipboard() const {
    const char* text = glfwGetClipboardString(handle_);
    return text != nullptr ? std::string(text) : std::string();
}

void Window::setClipboard(std::string_view text) {
    const std::string copy(text);
    glfwSetClipboardString(handle_, copy.c_str());
}

void Window::setCursor(CursorShape shape) {
    if (shape == cursor_ || shape == CursorShape::Count) return;
    cursor_ = shape;
    if (shape == CursorShape::Arrow) {
        glfwSetCursor(handle_, nullptr);
        return;
    }

    GLFWcursor*& cursor = cursors_[static_cast<std::size_t>(shape)];
    if (cursor == nullptr) {
        int standard = GLFW_ARROW_CURSOR;
        switch (shape) {
            case CursorShape::IBeam: standard = GLFW_IBEAM_CURSOR; break;
            case CursorShape::Hand: standard = GLFW_POINTING_HAND_CURSOR; break;
            case CursorShape::Crosshair: standard = GLFW_CROSSHAIR_CURSOR; break;
            case CursorShape::ResizeHorizontal: standard = GLFW_RESIZE_EW_CURSOR; break;
            case CursorShape::ResizeVertical: standard = GLFW_RESIZE_NS_CURSOR; break;
            case CursorShape::ResizeAll: standard = GLFW_RESIZE_ALL_CURSOR; break;
            case CursorShape::NotAllowed: standard = GLFW_NOT_ALLOWED_CURSOR; break;
            default: break;
        }
        cursor = glfwCreateStandardCursor(standard);
    }
    glfwSetCursor(handle_, cursor);
}

float Window::contentScale() const {
    float x = 1.0f;
    float y = 1.0f;
    glfwGetWindowContentScale(handle_, &x, &y);
    return x;
}

bool Window::shouldClose() const { return glfwWindowShouldClose(handle_) == GLFW_TRUE; }

void Window::setShouldClose(bool close) { glfwSetWindowShouldClose(handle_, close ? GLFW_TRUE : GLFW_FALSE); }

}
