#include "platform/InputScript.hpp"

#include "platform/Files.hpp"
#include "platform/Input.hpp"

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace cinder::platform {

namespace {

std::string_view trim(std::string_view text) {
    while (!text.empty() && (text.front() == ' ' || text.front() == '\t')) text.remove_prefix(1);
    while (!text.empty() && (text.back() == ' ' || text.back() == '\t' || text.back() == '\r')) text.remove_suffix(1);
    return text;
}

std::string_view word(std::string_view& text) {
    text = trim(text);
    const std::size_t end = std::min(text.find_first_of(" \t"), text.size());
    const std::string_view result = text.substr(0, end);
    text.remove_prefix(end);
    return result;
}

template <typename T>
T number(std::string_view text, int line) {
    T value{};
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (error != std::errc() || end != text.data() + text.size()) {
        throw std::runtime_error("input script line " + std::to_string(line) + ": \"" + std::string(text)
                                 + "\" is not a number");
    }
    return value;
}

char32_t decode(std::string_view text, std::size_t& offset) {
    const auto lead = static_cast<unsigned char>(text[offset]);
    int length = lead >= 0xF0 ? 4 : lead >= 0xE0 ? 3 : lead >= 0xC0 ? 2 : 1;
    std::uint32_t value = length == 1 ? lead : lead & (0xFFu >> (length + 1));
    for (int i = 1; i < length && offset + static_cast<std::size_t>(i) < text.size(); ++i) {
        value = (value << 6) | (static_cast<unsigned char>(text[offset + static_cast<std::size_t>(i)]) & 0x3Fu);
    }
    offset += static_cast<std::size_t>(length);
    return static_cast<char32_t>(value);
}

}

InputScript InputScript::load(const std::filesystem::path& path) {
    try {
        return parse(readTextFile(path));
    } catch (const std::runtime_error& error) {
        throw std::runtime_error(path.string() + ": " + error.what());
    }
}

InputScript InputScript::parse(std::string_view text) {
    InputScript script;
    int lineNumber = 0;
    std::size_t start = 0;
    while (start <= text.size()) {
        const std::size_t end = std::min(text.find('\n', start), text.size());
        std::string_view line = trim(text.substr(start, end - start));
        start = end + 1;
        ++lineNumber;
        if (line.empty() || line.front() == '#') continue;

        const int frame = number<int>(word(line), lineNumber);
        const std::string_view verb = word(line);
        script.last_ = std::max(script.last_, frame);

        const auto push = [&](InputEventType type, int code) {
            InputEvent event;
            event.type = type;
            event.code = code;
            script.steps_.push_back(Step{frame, event});
            return &script.steps_.back().event;
        };
        const auto key = [&]() {
            const std::string_view name = word(line);
            const int code = Input::keyCode(name);
            if (code < 0) throw std::runtime_error("input script line " + std::to_string(lineNumber) + ": unknown key");
            return code;
        };
        const auto button = [&]() {
            const std::string_view name = word(line);
            const int code = Input::buttonCode(name);
            if (code < 0) throw std::runtime_error("input script line " + std::to_string(lineNumber) + ": unknown button");
            return code;
        };

        if (verb == "move") {
            const float x = number<float>(word(line), lineNumber);
            const float y = number<float>(word(line), lineNumber);
            push(InputEventType::MouseMove, 0)->position = {x, y};
        } else if (verb == "down") {
            push(InputEventType::MouseDown, button());
        } else if (verb == "up") {
            push(InputEventType::MouseUp, button());
        } else if (verb == "click") {
            const int code = button();
            push(InputEventType::MouseDown, code);
            push(InputEventType::MouseUp, code);
        } else if (verb == "press") {
            push(InputEventType::KeyDown, key());
        } else if (verb == "release") {
            push(InputEventType::KeyUp, key());
        } else if (verb == "tap") {
            const int code = key();
            push(InputEventType::KeyDown, code);
            push(InputEventType::KeyUp, code);
        } else if (verb == "type") {
            const std::string_view typed = trim(line);
            for (std::size_t offset = 0; offset < typed.size();) push(InputEventType::Char, 0)->character = decode(typed, offset);
        } else if (verb == "scroll") {
            const float x = number<float>(word(line), lineNumber);
            const float y = number<float>(word(line), lineNumber);
            push(InputEventType::Wheel, 0)->wheel = {x, y};
        } else if (verb == "capture") {
            script.captures_.push_back(Capture{frame, std::string(trim(line))});
        } else if (verb == "quit") {
            script.quit_ = frame;
        } else {
            throw std::runtime_error("input script line " + std::to_string(lineNumber) + ": unknown action \""
                                     + std::string(verb) + "\"");
        }
    }
    std::stable_sort(script.steps_.begin(), script.steps_.end(),
                     [](const Step& a, const Step& b) { return a.frame < b.frame; });
    return script;
}

void InputScript::apply(Input& input, int frame) const {
    for (const Step& step : steps_) {
        if (step.frame == frame) input.inject(step.event);
    }
}

std::optional<std::string> InputScript::capture(int frame) const {
    for (const Capture& capture : captures_) {
        if (capture.frame == frame) return capture.path;
    }
    return std::nullopt;
}

}
