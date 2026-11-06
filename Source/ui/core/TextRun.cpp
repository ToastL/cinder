#include "ui/core/TextRun.hpp"

#include "text/FontSet.hpp"
#include "text/GlyphAtlas.hpp"
#include "ui/core/ElementList.hpp"

#include <algorithm>
#include <cmath>

namespace cinder::ui {

const std::vector<cinder::text::ShapedText>& TextRun::shape(const cinder::text::FontSet& fonts,
                                                            std::string_view text, const FontInfo& font) {
    if (valid_ && text == text_ && font == font_) return lines_;
    valid_ = true;
    text_ = std::string(text);
    font_ = font;
    lines_.clear();

    const cinder::text::Font& face = fonts.get(font.face);
    const cinder::text::FontMetrics metrics = face.metrics(font.size);
    lineHeight_ = std::ceil(metrics.lineHeight);
    ascent_ = std::round(metrics.ascent);

    std::size_t start = 0;
    float width = 0.0f;
    while (true) {
        const std::size_t end = text_.find('\n', start);
        const std::string_view line = std::string_view(text_).substr(start, end == std::string::npos ? std::string::npos : end - start);
        lines_.push_back(cinder::text::shape(face, line, font.size));
        width = std::max(width, lines_.back().width);
        if (end == std::string::npos) break;
        start = end + 1;
    }
    size_ = glm::vec2(std::ceil(width), lineHeight_ * static_cast<float>(lines_.size()));
    return lines_;
}

cinder::text::ShapedText TextRun::scaled(const cinder::text::ShapedText& shaped, float scale) {
    if (scale == 1.0f) return shaped;
    cinder::text::ShapedText result = shaped;
    result.pixelSize *= scale;
    result.width *= scale;
    for (cinder::text::ShapedGlyph& glyph : result.glyphs) {
        glyph.advance *= scale;
        glyph.offset *= scale;
    }
    return result;
}

void TextRun::paint(ElementList& list, int layer, glm::vec2 topLeft, float scale, Color color,
                    cinder::text::GlyphAtlas& atlas, float alignWidth, int alignment) const {
    for (std::size_t i = 0; i < lines_.size(); ++i) {
        float x = topLeft.x;
        if (alignWidth >= 0.0f && alignment != 0) {
            const float free = alignWidth - lines_[i].width * scale;
            x += alignment == 1 ? std::round(free * 0.5f) : free;
        }
        const glm::vec2 baseline(x, topLeft.y + (ascent_ + lineHeight_ * static_cast<float>(i)) * scale);
        if (scale == 1.0f) {
            list.text(layer, baseline, lines_[i], color, atlas);
        } else {
            list.text(layer, baseline, scaled(lines_[i], scale), color, atlas);
        }
    }
}

}
