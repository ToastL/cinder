#pragma once

#include "platform/Log.hpp"
#include "ui/core/Widget.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace cinder::ui {
class Application;
class ScrollBox;
}

namespace cinder::dev::panels {

class Console {
public:
    static constexpr std::size_t MAX_LINES = 2000;

    explicit Console(cinder::ui::Application& app);
    ~Console();

    Console(const Console&) = delete;
    Console& operator=(const Console&) = delete;

    const std::shared_ptr<cinder::ui::Widget>& widget() const { return widget_; }

    void push(cinder::platform::LogLevel level, std::string_view text);
    void update();
    void clear();
    std::size_t lines() const { return count_; }

private:
    struct Line {
        cinder::platform::LogLevel level;
        std::string text;
    };

    void build();

    cinder::ui::Application& app_;
    std::shared_ptr<cinder::ui::Widget> widget_;
    std::shared_ptr<cinder::ui::ScrollBox> scroll_;
    std::vector<Line> pending_;
    std::size_t count_ = 0;
};

}
