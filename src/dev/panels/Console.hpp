#pragma once

#include "platform/Log.hpp"
#include "ui/core/Widget.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace cinder::script { class LuaHost; }
namespace cinder::ui {
class Application;
class ScrollBox;
class TextBox;
}

namespace cinder::dev {
class History;
class Selection;
}

namespace cinder::dev::panels {

class Console {
public:
    static constexpr std::size_t MAX_LINES = 2000;

    Console(cinder::script::LuaHost& script, History& history, Selection& selection, cinder::ui::Application& app);
    ~Console();

    Console(const Console&) = delete;
    Console& operator=(const Console&) = delete;

    const std::shared_ptr<cinder::ui::Widget>& widget() const { return widget_; }
    const std::shared_ptr<cinder::ui::TextBox>& input() const { return input_; }

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
    void submit(const std::string& source);
    bool recall(int direction);

    cinder::script::LuaHost& script_;
    History& changes_;
    Selection& selection_;
    cinder::ui::Application& app_;
    std::shared_ptr<cinder::ui::Widget> widget_;
    std::shared_ptr<cinder::ui::ScrollBox> scroll_;
    std::shared_ptr<cinder::ui::TextBox> input_;
    std::vector<Line> pending_;
    std::vector<std::string> history_;
    std::size_t count_ = 0;
    int historyPos_ = -1;
};

}
