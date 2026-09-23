#pragma once

#include <glm/vec2.hpp>

#include <cstdint>
#include <string_view>
#include <vector>

namespace cinder::text {

class Font;

struct ShapedGlyph {
    std::uint32_t glyph = 0;
    std::uint32_t cluster = 0;
    float advance = 0.0f;
    glm::vec2 offset{0.0f};
};

struct ShapedText {
    const Font* font = nullptr;
    float pixelSize = 0.0f;
    std::vector<ShapedGlyph> glyphs;
    float width = 0.0f;
};

ShapedText shape(const Font& font, std::string_view utf8, float pixelSize);

}
