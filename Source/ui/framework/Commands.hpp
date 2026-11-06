#pragma once

#include "ui/core/Events.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace cinder::ui {

struct Shortcut {
    int key = -1;
    std::uint8_t modifiers = 0;

    static Shortcut primary(int key, std::uint8_t extra = 0) {
        return {key, static_cast<std::uint8_t>(cinder::platform::modifiers::PRIMARY | extra)};
    }

    bool valid() const { return key >= 0; }
    bool matches(const KeyEvent& event) const;
    std::string label() const;

    bool operator==(const Shortcut&) const = default;
};

struct Command {
    std::string name;
    std::string label;
    std::string description;
    Shortcut shortcut;
};

struct CommandAction {
    std::function<void()> execute;
    std::function<bool()> canExecute;
    std::function<bool()> isChecked;

    bool can() const { return !canExecute || canExecute(); }
    bool checked() const { return isChecked && isChecked(); }
};

class CommandList {
public:
    void map(std::shared_ptr<const Command> command, CommandAction action);
    bool process(const KeyEvent& event) const;
    bool execute(const Command& command) const;
    const CommandAction* action(const Command& command) const;

private:
    std::vector<std::pair<std::shared_ptr<const Command>, CommandAction>> bindings_;
};

}
