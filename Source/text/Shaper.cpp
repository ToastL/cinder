#include "text/Shaper.hpp"

#include "text/Font.hpp"

#include <hb.h>

namespace cinder::text {

ShapedText shape(const Font& font, std::string_view utf8, float pixelSize) {
    ShapedText shaped;
    shaped.font = &font;
    shaped.pixelSize = pixelSize;
    if (utf8.empty()) return shaped;

    hb_buffer_t* buffer = hb_buffer_create();
    const int length = static_cast<int>(utf8.size());
    hb_buffer_add_utf8(buffer, utf8.data(), length, 0, length);
    hb_buffer_guess_segment_properties(buffer);
    hb_shape(font.shaper(), buffer, nullptr, 0);

    unsigned count = 0;
    const hb_glyph_info_t* infos = hb_buffer_get_glyph_infos(buffer, &count);
    const hb_glyph_position_t* positions = hb_buffer_get_glyph_positions(buffer, &count);
    const float scale = pixelSize / font.unitsPerEm();

    shaped.glyphs.reserve(count);
    for (unsigned i = 0; i < count; ++i) {
        ShapedGlyph glyph;
        glyph.glyph = infos[i].codepoint;
        glyph.cluster = infos[i].cluster;
        glyph.advance = static_cast<float>(positions[i].x_advance) * scale;
        glyph.offset = glm::vec2(static_cast<float>(positions[i].x_offset) * scale,
                                 -static_cast<float>(positions[i].y_offset) * scale);
        shaped.width += glyph.advance;
        shaped.glyphs.push_back(glyph);
    }

    hb_buffer_destroy(buffer);
    return shaped;
}

}
