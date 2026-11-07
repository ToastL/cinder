#pragma once

#include "Platform/Cursor.hpp"

#include <string>
#include <string_view>

namespace cinder::UI {

class PlatformHooks {
public:
    virtual ~PlatformHooks() = default;

    virtual std::string clipboard() const = 0;
    virtual void setClipboard(std::string_view text) = 0;
    virtual void setCursor(cinder::Platform::CursorShape shape) = 0;
    virtual double time() const = 0;
};

class HeadlessPlatform final : public PlatformHooks {
public:
    std::string clipboard() const override { return clipboard_; }
    void setClipboard(std::string_view text) override { clipboard_ = std::string(text); }
    void setCursor(cinder::Platform::CursorShape shape) override { cursor_ = shape; }
    double time() const override { return time_; }

    void advance(double seconds) { time_ += seconds; }
    cinder::Platform::CursorShape cursor() const { return cursor_; }

private:
    std::string clipboard_;
    cinder::Platform::CursorShape cursor_ = cinder::Platform::CursorShape::Arrow;
    double time_ = 0.0;
};

}
