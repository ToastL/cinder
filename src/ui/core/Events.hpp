#pragma once

#include "platform/InputEvent.hpp"

#include <glm/vec2.hpp>

#include <cstdint>

namespace cinder::ui {

struct Modified {
    std::uint8_t modifiers = 0;

    bool shift() const { return (modifiers & cinder::platform::modifiers::SHIFT) != 0; }
    bool control() const { return (modifiers & cinder::platform::modifiers::CONTROL) != 0; }
    bool alt() const { return (modifiers & cinder::platform::modifiers::ALT) != 0; }
    bool super() const { return (modifiers & cinder::platform::modifiers::SUPER) != 0; }
    bool primary() const { return (modifiers & cinder::platform::modifiers::PRIMARY) != 0; }
};

struct PointerEvent : Modified {
    glm::vec2 position{0.0f};
    glm::vec2 delta{0.0f};
    glm::vec2 wheel{0.0f};
    int button = -1;
    std::uint32_t buttons = 0;

    bool isDown(int which) const { return (buttons & (1u << static_cast<unsigned>(which))) != 0; }
};

struct KeyEvent : Modified {
    int key = 0;
    bool repeat = false;
};

struct CharEvent : Modified {
    char32_t character = 0;
};

enum class FocusCause : std::uint8_t { Mouse, Navigation, SetDirectly, Cleared, WindowLost };

struct FocusEvent {
    FocusCause cause = FocusCause::SetDirectly;
};

}
