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
    if (handle_ != nullptr) glfwDestroyWindow(handle_);
    Glfw::release();
}

bool Window::shouldClose() const { return glfwWindowShouldClose(handle_) == GLFW_TRUE; }

void Window::setShouldClose(bool close) { glfwSetWindowShouldClose(handle_, close ? GLFW_TRUE : GLFW_FALSE); }

}
