#pragma once

#include "platform/Glfw.hpp"
#include "platform/Window.hpp"
#include "ui/framework/PlatformHooks.hpp"

namespace cinder::ui {

class WindowPlatform final : public PlatformHooks {
public:
    explicit WindowPlatform(cinder::platform::Window& window) : window_(window) {}

    std::string clipboard() const override { return window_.clipboard(); }
    void setClipboard(std::string_view text) override { window_.setClipboard(text); }
    void setCursor(cinder::platform::CursorShape shape) override { window_.setCursor(shape); }
    double time() const override { return cinder::platform::Glfw::time(); }

private:
    cinder::platform::Window& window_;
};

}
