#pragma once

#include "Platform/Glfw.hpp"
#include "Platform/Window.hpp"
#include "UI/Framework/PlatformHooks.hpp"

namespace cinder::UI {

class WindowPlatform final : public PlatformHooks {
public:
    explicit WindowPlatform(cinder::Platform::Window& window) : window_(window) {}

    std::string clipboard() const override { return window_.clipboard(); }
    void setClipboard(std::string_view text) override { window_.setClipboard(text); }
    void setCursor(cinder::Platform::CursorShape shape) override { window_.setCursor(shape); }
    double time() const override { return cinder::Platform::Glfw::time(); }

private:
    cinder::Platform::Window& window_;
};

}
