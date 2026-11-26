#pragma once

#include "Text/TrueType.hpp"

#include <glm/vec2.hpp>

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace cinder::Text {

struct FontMetrics {
    float ascent = 0.0f;
    float descent = 0.0f;
    float lineHeight = 0.0f;
};

struct GlyphBitmap {
    int width = 0;
    int height = 0;
    glm::vec2 bearing{0.0f};
    std::vector<std::uint8_t> coverage;
};

class Font {
public:
    explicit Font(std::vector<std::uint8_t> bytes);

    Font(const Font&) = delete;
    Font& operator=(const Font&) = delete;

    static std::unique_ptr<Font> load(const std::filesystem::path& path);

    const std::string& family() const { return face_.family(); }
    float unitsPerEm() const { return static_cast<float>(face_.unitsPerEm()); }
    FontMetrics metrics(float pixelSize) const;
    std::uint32_t glyphIndex(char32_t codepoint) const { return face_.glyphIndex(codepoint); }
    float advance(std::uint32_t glyph) const { return static_cast<float>(face_.advance(glyph)); }
    float kerning(std::uint32_t left, std::uint32_t right) const {
        return static_cast<float>(face_.kerning(left, right));
    }
    GlyphBitmap rasterize(std::uint32_t glyph, float pixelSize) const;

private:
    std::vector<std::uint8_t> bytes_;
    TrueType face_;
};

}
