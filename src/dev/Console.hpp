#pragma once

#include "platform/Log.hpp"

#include <deque>
#include <string>
#include <string_view>
#include <vector>

struct ImGuiInputTextCallbackData;

namespace cinder::script { class LuaHost; }

namespace cinder::dev {

class History;
class Selection;

class Console {
public:
    static constexpr std::size_t MAX_LINES = 2000;
    static constexpr const char* TITLE = "Console";

    Console(cinder::script::LuaHost& script, History& history, Selection& selection);
    ~Console();

    Console(const Console&) = delete;
    Console& operator=(const Console&) = delete;

    void push(cinder::platform::LogLevel level, std::string_view text);
    void draw();

    bool open() const { return open_; }
    void setOpen(bool open) { open_ = open; }

private:
    struct Line {
        cinder::platform::LogLevel level;
        std::string text;
    };

    static int onHistory(ImGuiInputTextCallbackData* data);

    void submit();

    cinder::script::LuaHost& script_;
    History& changes_;
    Selection& selection_;
    std::deque<Line> lines_;
    std::vector<std::string> history_;
    char input_[512] = {};
    int historyPos_ = -1;
    bool scrollToBottom_ = false;
    bool open_ = true;
};

}
