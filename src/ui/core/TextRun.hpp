#pragma once

#include "text/Shaper.hpp"
#include "ui/core/Color.hpp"
#include "ui/core/Style.hpp"

#include <glm/vec2.hpp>

#include <string>
#include <string_view>
#include <vector>

namespace cinder::text {
class FontSet;
class GlyphAtlas;
}

namespace cinder::ui {

class ElementList;

class TextRun {
public:
    const std::vector<cinder::text::ShapedText>& shape(const cinder::text::FontSet& fonts, std::string_view text,
                                                       const FontInfo& font);

    glm::vec2 size() const { return size_; }
    float lineHeight() const { return lineHeight_; }
    float ascent() const { return ascent_; }
    const std::vector<cinder::text::ShapedText>& lines() const { return lines_; }

    void paint(ElementList& list, int layer, glm::vec2 topLeft, float scale, Color color,
               cinder::text::GlyphAtlas& atlas, float alignWidth = -1.0f, int alignment = 0) const;

    static cinder::text::ShapedText scaled(const cinder::text::ShapedText& shaped, float scale);

private:
    std::string text_;
    FontInfo font_;
    bool valid_ = false;
    std::vector<cinder::text::ShapedText> lines_;
    glm::vec2 size_{0.0f};
    float lineHeight_ = 0.0f;
    float ascent_ = 0.0f;
};

}
