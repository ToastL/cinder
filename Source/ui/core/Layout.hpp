#pragma once

#include <glm/vec2.hpp>

#include <algorithm>
#include <cstdint>

namespace cinder::ui {

struct Margin {
    float left = 0.0f;
    float top = 0.0f;
    float right = 0.0f;
    float bottom = 0.0f;

    constexpr Margin() = default;
    constexpr Margin(float all) : left(all), top(all), right(all), bottom(all) {}
    constexpr Margin(float horizontal, float vertical)
        : left(horizontal), top(vertical), right(horizontal), bottom(vertical) {}
    constexpr Margin(float left, float top, float right, float bottom)
        : left(left), top(top), right(right), bottom(bottom) {}

    glm::vec2 total() const { return {left + right, top + bottom}; }
    glm::vec2 topLeft() const { return {left, top}; }

    bool operator==(const Margin&) const = default;
};

enum class HAlign : std::uint8_t { Fill, Left, Center, Right };
enum class VAlign : std::uint8_t { Fill, Top, Center, Bottom };
enum class Orientation : std::uint8_t { Horizontal, Vertical };

struct Span {
    float offset = 0.0f;
    float size = 0.0f;
};

inline Span alignSpan(int alignment, float allotted, float desired, float before, float after) {
    const float available = std::max(0.0f, allotted - before - after);
    if (alignment == 0) return {before, available};
    const float size = std::min(desired, available);
    if (alignment == 1) return {before, size};
    if (alignment == 2) return {before + (available - size) * 0.5f, size};
    return {before + available - size, size};
}

inline Span alignHorizontal(HAlign alignment, float allotted, float desired, const Margin& padding) {
    return alignSpan(static_cast<int>(alignment), allotted, desired, padding.left, padding.right);
}

inline Span alignVertical(VAlign alignment, float allotted, float desired, const Margin& padding) {
    return alignSpan(static_cast<int>(alignment), allotted, desired, padding.top, padding.bottom);
}

}
