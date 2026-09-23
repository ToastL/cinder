#pragma once

#include "platform/InputEvent.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace cinder::platform {

class Input;

class InputScript {
public:
    static InputScript load(const std::filesystem::path& path);
    static InputScript parse(std::string_view text);

    void apply(Input& input, int frame) const;
    std::optional<std::string> capture(int frame) const;
    bool quits(int frame) const { return quit_ >= 0 && frame >= quit_; }
    bool closes(int frame) const;
    int lastFrame() const { return last_; }
    bool empty() const { return steps_.empty() && captures_.empty() && closes_.empty() && quit_ < 0; }

private:
    struct Step {
        int frame = 0;
        InputEvent event;
    };

    struct Capture {
        int frame = 0;
        std::string path;
    };

    std::vector<Step> steps_;
    std::vector<Capture> captures_;
    std::vector<int> closes_;
    int quit_ = -1;
    int last_ = 0;
};

}
