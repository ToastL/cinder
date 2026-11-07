#pragma once

#include "Platform/Log.hpp"
#include "UI/Core/Widget.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace cinder::UI {
class Application;
class ScrollBox;
}

namespace cinder::Dev::Panels {

class Console {
public:
    static constexpr std::size_t MAX_LINES = 2000;

    explicit Console(cinder::UI::Application& app);
    ~Console();

    Console(const Console&) = delete;
    Console& operator=(const Console&) = delete;

    const std::shared_ptr<cinder::UI::Widget>& widget() const { return widget_; }

    void push(cinder::Platform::LogLevel level, std::string_view text);
    void update();
    void clear();
    std::size_t lines() const { return count_; }

private:
    struct Line {
        cinder::Platform::LogLevel level;
        std::string text;
    };

    void build();

    cinder::UI::Application& app_;
    std::shared_ptr<cinder::UI::Widget> widget_;
    std::shared_ptr<cinder::UI::ScrollBox> scroll_;
    std::vector<Line> pending_;
    std::size_t count_ = 0;
};

}
