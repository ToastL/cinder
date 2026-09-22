#include "ui/core/ElementList.hpp"

#include "text/GlyphAtlas.hpp"
#include "text/Shaper.hpp"

#include <glm/common.hpp>

#include <algorithm>

namespace cinder::ui {

void ElementList::reset(glm::vec2 size, float scale) {
    size_ = size;
    scale_ = scale > 0.0f ? scale : 1.0f;
    clips_.assign(1, bounds());
    transforms_.assign(1, Transform2D{});
    elements_.clear();
    names_.clear();
    atlas_ = nullptr;
}

void ElementList::pushClip(const Rect& rect) { clips_.push_back(clips_.back().intersect(rect)); }

void ElementList::popClip() {
    if (clips_.size() > 1) clips_.pop_back();
}

void ElementList::pushTransform(const Transform2D& transform) {
    transforms_.push_back(transform.then(transforms_.back()));
}

void ElementList::popTransform() {
    if (transforms_.size() > 1) transforms_.pop_back();
}

Element& ElementList::add(int layer) {
    Element& element = elements_.emplace_back();
    element.layer = layer;
    element.clip = clips_.back();
    element.transform = transforms_.back();
    return element;
}

void ElementList::box(int layer, const Rect& rect, const BoxStyle& style) {
    if (rect.empty()) return;
    add(layer).shape = BoxElement{rect, style};
}

void ElementList::image(int layer, const Rect& rect, TextureRef texture, Color tint, glm::vec2 uvMin,
                        glm::vec2 uvMax, bool opaque) {
    if (rect.empty()) return;
    add(layer).shape = ImageElement{rect, texture, tint, uvMin, uvMax, opaque};
}

void ElementList::text(int layer, glm::vec2 baseline, const cinder::text::ShapedText& shaped, Color color,
                       cinder::text::GlyphAtlas& atlas) {
    if (shaped.font == nullptr || shaped.glyphs.empty()) return;
    atlas_ = &atlas;

    TextElement element;
    element.color = color;
    element.glyphs.reserve(shaped.glyphs.size());

    const float pixelSize = shaped.pixelSize * scale_;
    const float page = static_cast<float>(cinder::text::GlyphAtlas::PAGE_SIZE);
    float pen = 0.0f;
    for (const cinder::text::ShapedGlyph& glyph : shaped.glyphs) {
        const cinder::text::AtlasGlyph& placed = atlas.glyph(*shaped.font, glyph.glyph, pixelSize);
        if (!placed.empty()) {
            const glm::vec2 origin = glm::round((baseline + glm::vec2(pen, 0.0f) + glyph.offset) * scale_);
            const glm::vec2 corner = origin + placed.bearing;
            const glm::vec2 extent(static_cast<float>(placed.rect.width), static_cast<float>(placed.rect.height));
            const glm::vec2 texel(static_cast<float>(placed.rect.x), static_cast<float>(placed.rect.y));
            element.glyphs.push_back(GlyphQuad{placed.page, Rect::fromSize(corner / scale_, extent / scale_),
                                               texel / page, (texel + extent) / page});
        }
        pen += glyph.advance;
    }
    if (element.glyphs.empty()) return;
    add(layer).shape = std::move(element);
}

void ElementList::lines(int layer, std::span<const glm::vec2> points, Color color, float thickness,
                        bool closed) {
    if (points.size() < 2 || thickness <= 0.0f) return;
    add(layer).shape = LinesElement{std::vector<glm::vec2>(points.begin(), points.end()), color, thickness, closed};
}

void ElementList::polygon(int layer, std::span<const glm::vec2> points, Color color) {
    if (points.size() < 3) return;
    add(layer).shape = PolygonElement{std::vector<glm::vec2>(points.begin(), points.end()), color};
}

void ElementList::mesh(int layer, std::span<const glm::vec2> positions, std::span<const Color> colors,
                       std::span<const std::uint32_t> indices) {
    if (positions.empty() || colors.size() != positions.size() || indices.size() < 3) return;
    add(layer).shape = MeshElement{std::vector<glm::vec2>(positions.begin(), positions.end()),
                                   std::vector<Color>(colors.begin(), colors.end()),
                                   std::vector<std::uint32_t>(indices.begin(), indices.end())};
}

void ElementList::gradient(int layer, const Rect& rect, Color topLeft, Color topRight, Color bottomRight,
                           Color bottomLeft) {
    if (rect.empty()) return;
    const glm::vec2 positions[] = {rect.min, {rect.max.x, rect.min.y}, rect.max, {rect.min.x, rect.max.y}};
    const Color colors[] = {topLeft, topRight, bottomRight, bottomLeft};
    const std::uint32_t indices[] = {0, 1, 2, 0, 2, 3};
    mesh(layer, positions, colors, indices);
}

TextureRef ElementList::named(std::string_view path) {
    const auto found = std::find(names_.begin(), names_.end(), path);
    const auto index = static_cast<std::uint32_t>(found - names_.begin());
    if (found == names_.end()) names_.emplace_back(path);
    return TextureRef{TextureRef::Kind::Named, index};
}

}
