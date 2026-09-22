#include "ui/framework/Commands.hpp"

namespace cinder::ui {

namespace {

constexpr std::uint8_t CHORD_MODIFIERS = cinder::platform::modifiers::SHIFT | cinder::platform::modifiers::CONTROL
        | cinder::platform::modifiers::ALT | cinder::platform::modifiers::SUPER;

std::string keyName(int key) {
    namespace keys = cinder::platform::keys;
    if (key >= keys::A && key <= keys::Z) return std::string(1, static_cast<char>('A' + (key - keys::A)));
    if (key >= keys::DIGIT_0 && key <= keys::DIGIT_0 + 9) return std::string(1, static_cast<char>('0' + (key - keys::DIGIT_0)));
    if (key >= keys::F1 && key < keys::F1 + 12) return "F" + std::to_string(key - keys::F1 + 1);
    switch (key) {
        case keys::SPACE: return "Space";
        case keys::ESCAPE: return "Esc";
        case keys::ENTER: return "Enter";
        case keys::TAB: return "Tab";
        case keys::BACKSPACE: return "Backspace";
        case keys::DELETE: return "Delete";
        case keys::LEFT: return "Left";
        case keys::RIGHT: return "Right";
        case keys::UP: return "Up";
        case keys::DOWN: return "Down";
        case keys::COMMA: return ",";
        case keys::PERIOD: return ".";
        case keys::MINUS: return "-";
        case keys::EQUAL: return "=";
        default: return "?";
    }
}

}

bool Shortcut::matches(const KeyEvent& event) const {
    return valid() && event.key == key && (event.modifiers & CHORD_MODIFIERS) == modifiers;
}

std::string Shortcut::label() const {
    if (!valid()) return {};
    namespace mods = cinder::platform::modifiers;
    std::string text;
#if defined(__APPLE__)
    if ((modifiers & mods::CONTROL) != 0) text += "\xE2\x8C\x83";
    if ((modifiers & mods::ALT) != 0) text += "\xE2\x8C\xA5";
    if ((modifiers & mods::SHIFT) != 0) text += "\xE2\x87\xA7";
    if ((modifiers & mods::SUPER) != 0) text += "\xE2\x8C\x98";
    return text + keyName(key);
#else
    if ((modifiers & mods::CONTROL) != 0) text += "Ctrl+";
    if ((modifiers & mods::ALT) != 0) text += "Alt+";
    if ((modifiers & mods::SHIFT) != 0) text += "Shift+";
    if ((modifiers & mods::SUPER) != 0) text += "Super+";
    return text + keyName(key);
#endif
}

void CommandList::map(std::shared_ptr<const Command> command, CommandAction action) {
    for (auto& binding : bindings_) {
        if (binding.first == command) {
            binding.second = std::move(action);
            return;
        }
    }
    bindings_.emplace_back(std::move(command), std::move(action));
}

bool CommandList::process(const KeyEvent& event) const {
    for (const auto& [command, action] : bindings_) {
        if (!command->chord.matches(event)) continue;
        if (!action.can()) return true;
        if (action.execute) action.execute();
        return true;
    }
    return false;
}

bool CommandList::execute(const Command& command) const {
    const CommandAction* found = action(command);
    if (found == nullptr || !found->can() || !found->execute) return false;
    found->execute();
    return true;
}

const CommandAction* CommandList::action(const Command& command) const {
    for (const auto& binding : bindings_) {
        if (binding.first.get() == &command) return &binding.second;
    }
    return nullptr;
}

}
