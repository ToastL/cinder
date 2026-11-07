#pragma once

#include "Text/Shaper.hpp"
#include "UI/Core/Color.hpp"
#include "UI/Core/Style.hpp"

#include <glm/vec2.hpp>

#include <string>
#include <string_view>
#include <vector>

namespace cinder::Text {
class FontSet;
class GlyphAtlas;
}

namespace cinder::UI {

class ElementList;

class TextRun {
public:
    const std::vector<cinder::Text::ShapedText>& shape(const cinder::Text::FontSet& fonts, std::string_view text,
                                                       const FontInfo& font);

    glm::vec2 size() const { return size_; }
    float lineHeight() const { return lineHeight_; }
    float ascent() const { return ascent_; }
    const std::vector<cinder::Text::ShapedText>& lines() const { return lines_; }

    void paint(ElementList& list, int layer, glm::vec2 topLeft, float scale, Color color,
               cinder::Text::GlyphAtlas& atlas, float alignWidth = -1.0f, int alignment = 0) const;

    static cinder::Text::ShapedText scaled(const cinder::Text::ShapedText& shaped, float scale);

private:
    std::string text_;
    FontInfo font_;
    bool valid_ = false;
    std::vector<cinder::Text::ShapedText> lines_;
    glm::vec2 size_{0.0f};
    float lineHeight_ = 0.0f;
    float ascent_ = 0.0f;
};

}
