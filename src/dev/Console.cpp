#include "dev/Console.hpp"

#include "script/LuaHost.hpp"

#include <imgui.h>

namespace cinder::dev {
namespace {

const ImVec4 ERROR_COLOR{1.0f, 0.45f, 0.4f, 1.0f};

}

Console::Console(cinder::script::LuaHost& script) : script_(script) {
    cinder::platform::setLogSink(
            [this](cinder::platform::LogLevel level, std::string_view text) {
                push(level, text);
            });
}

void Console::push(cinder::platform::LogLevel level, std::string_view text) {
    lines_.push_back(Line{level, std::string(text)});
    while (lines_.size() > MAX_LINES) lines_.pop_front();
    scrollToBottom_ = true;
}

int Console::onHistory(ImGuiInputTextCallbackData* data) {
    Console& console = *static_cast<Console*>(data->UserData);
    if (console.history_.empty()) return 0;

    const int last = static_cast<int>(console.history_.size()) - 1;
    const int previous = console.historyPos_;

    if (data->EventKey == ImGuiKey_UpArrow) {
        if (console.historyPos_ < 0) console.historyPos_ = last;
        else if (console.historyPos_ > 0) --console.historyPos_;
    } else if (data->EventKey == ImGuiKey_DownArrow) {
        if (console.historyPos_ >= 0 && ++console.historyPos_ > last) console.historyPos_ = -1;
    }

    if (console.historyPos_ == previous) return 0;

    data->DeleteChars(0, data->BufTextLen);
    if (console.historyPos_ >= 0) {
        data->InsertChars(0, console.history_[console.historyPos_].c_str());
    }
    return 0;
}

void Console::submit() {
    std::string source(input_);
    input_[0] = '\0';
    historyPos_ = -1;

    while (!source.empty() && source.back() == ' ') source.pop_back();
    if (source.empty()) return;

    if (history_.empty() || history_.back() != source) history_.push_back(source);

    push(cinder::platform::LogLevel::Info, "> " + source);
    script_.eval(source);
}

void Console::draw() {
    if (!open_) return;

    ImGui::SetNextWindowSize(ImVec2(720, 300), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin(TITLE, &open_, ImGuiWindowFlags_NoFocusOnAppearing)) {
        ImGui::End();
        return;
    }

    if (ImGui::SmallButton("Clear")) lines_.clear();
    ImGui::SameLine();
    ImGui::TextDisabled("%zu lines", lines_.size());
    ImGui::Separator();

    const float reserved = ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y;
    ImGui::BeginChild("scroll", ImVec2(0.0f, -reserved), ImGuiChildFlags_None,
                      ImGuiWindowFlags_HorizontalScrollbar);

    for (const Line& line : lines_) {
        const bool error = line.level == cinder::platform::LogLevel::Error;
        if (error) ImGui::PushStyleColor(ImGuiCol_Text, ERROR_COLOR);
        ImGui::TextUnformatted(line.text.c_str());
        if (error) ImGui::PopStyleColor();
    }

    if (scrollToBottom_) {
        ImGui::SetScrollHereY(1.0f);
        scrollToBottom_ = false;
    }
    ImGui::EndChild();

    ImGui::Separator();
    ImGui::SetNextItemWidth(-1.0f);

    const ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue
            | ImGuiInputTextFlags_CallbackHistory;

    if (ImGui::InputText("##console", input_, sizeof(input_), flags, onHistory, this)) {
        submit();
        ImGui::SetKeyboardFocusHere(-1);
    }

    ImGui::End();
}

Console::~Console() { cinder::platform::setLogSink(nullptr); }

}
