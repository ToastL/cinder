#include "Text/Shaper.hpp"

#include "Platform/Utf8.hpp"
#include "Text/Font.hpp"

namespace cinder::Text {

ShapedText shape(const Font& font, std::string_view utf8, float pixelSize) {
    ShapedText shaped;
    shaped.font = &font;
    shaped.pixelSize = pixelSize;
    if (utf8.empty()) return shaped;

    const float scale = pixelSize / font.unitsPerEm();
    std::size_t offset = 0;
    while (offset < utf8.size()) {
        ShapedGlyph glyph;
        glyph.cluster = static_cast<std::uint32_t>(offset);
        glyph.glyph = font.glyphIndex(cinder::Platform::decodeUtf8(utf8, offset));
        glyph.advance = font.advance(glyph.glyph) * scale;
        if (!shaped.glyphs.empty()) {
            ShapedGlyph& previous = shaped.glyphs.back();
            const float kerning = font.kerning(previous.glyph, glyph.glyph) * scale;
            previous.advance += kerning;
            shaped.width += kerning;
        }
        shaped.width += glyph.advance;
        shaped.glyphs.push_back(glyph);
    }
    return shaped;
}

}
