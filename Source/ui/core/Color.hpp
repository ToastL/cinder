#pragma once

#include <glm/vec4.hpp>

#include <cmath>
#include <cstdint>

namespace cinder::ui {

struct Color {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 1.0f;

    static constexpr Color white() { return {1.0f, 1.0f, 1.0f, 1.0f}; }
    static constexpr Color black() { return {0.0f, 0.0f, 0.0f, 1.0f}; }
    static constexpr Color transparent() { return {0.0f, 0.0f, 0.0f, 0.0f}; }

    static float toLinear(float channel) {
        return channel <= 0.04045f ? channel / 12.92f : std::pow((channel + 0.055f) / 1.055f, 2.4f);
    }

    static float toSrgb(float channel) {
        return channel <= 0.0031308f ? channel * 12.92f : 1.055f * std::pow(channel, 1.0f / 2.4f) - 0.055f;
    }

    static Color srgb(std::uint8_t red, std::uint8_t green, std::uint8_t blue, std::uint8_t alpha = 255) {
        return {toLinear(red / 255.0f), toLinear(green / 255.0f), toLinear(blue / 255.0f), alpha / 255.0f};
    }

    static Color hex(std::uint32_t rgba) {
        return srgb(static_cast<std::uint8_t>(rgba >> 24), static_cast<std::uint8_t>(rgba >> 16),
                    static_cast<std::uint8_t>(rgba >> 8), static_cast<std::uint8_t>(rgba));
    }

    Color withAlpha(float alpha) const { return {r, g, b, alpha}; }
    Color operator*(const Color& other) const {
        return {r * other.r, g * other.g, b * other.b, a * other.a};
    }

    glm::vec4 vec() const { return {r, g, b, a}; }

    bool operator==(const Color&) const = default;
};

}
