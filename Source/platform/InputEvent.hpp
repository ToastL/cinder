#pragma once

#include <glm/vec2.hpp>

#include <cstdint>

namespace cinder::platform {

enum class InputEventType : std::uint8_t {
    KeyDown,
    KeyUp,
    Char,
    MouseDown,
    MouseUp,
    MouseMove,
    Wheel,
    CursorEnter,
    CursorLeave,
    FocusLost,
};

namespace modifiers {

constexpr std::uint8_t SHIFT = 1;
constexpr std::uint8_t CONTROL = 2;
constexpr std::uint8_t ALT = 4;
constexpr std::uint8_t SUPER = 8;

#if defined(__APPLE__)
constexpr std::uint8_t PRIMARY = SUPER;
#else
constexpr std::uint8_t PRIMARY = CONTROL;
#endif

}

namespace buttons {

constexpr int LEFT = 0;
constexpr int RIGHT = 1;
constexpr int MIDDLE = 2;
constexpr int COUNT = 8;

}

namespace keys {

constexpr int SPACE = 32;
constexpr int APOSTROPHE = 39;
constexpr int COMMA = 44;
constexpr int MINUS = 45;
constexpr int PERIOD = 46;
constexpr int SLASH = 47;
constexpr int DIGIT_0 = 48;
constexpr int SEMICOLON = 59;
constexpr int EQUAL = 61;
constexpr int A = 65;
constexpr int Z = 90;
constexpr int ESCAPE = 256;
constexpr int ENTER = 257;
constexpr int TAB = 258;
constexpr int BACKSPACE = 259;
constexpr int INSERT = 260;
constexpr int DELETE = 261;
constexpr int RIGHT = 262;
constexpr int LEFT = 263;
constexpr int DOWN = 264;
constexpr int UP = 265;
constexpr int PAGE_UP = 266;
constexpr int PAGE_DOWN = 267;
constexpr int HOME = 268;
constexpr int END = 269;
constexpr int F1 = 290;
constexpr int KEYPAD_ENTER = 335;
constexpr int LEFT_SHIFT = 340;
constexpr int LEFT_CONTROL = 341;
constexpr int LEFT_ALT = 342;
constexpr int LEFT_SUPER = 343;
constexpr int RIGHT_SHIFT = 344;
constexpr int RIGHT_CONTROL = 345;
constexpr int RIGHT_ALT = 346;
constexpr int RIGHT_SUPER = 347;
constexpr int COUNT = 349;

constexpr int letter(char c) { return A + (c - (c >= 'a' ? 'a' : 'A')); }
constexpr int digit(int d) { return DIGIT_0 + d; }

}

struct InputEvent {
    InputEventType type = InputEventType::MouseMove;
    int code = 0;
    std::uint8_t modifiers = 0;
    bool repeat = false;
    char32_t character = 0;
    glm::vec2 position{0.0f};
    glm::vec2 wheel{0.0f};
};

}
