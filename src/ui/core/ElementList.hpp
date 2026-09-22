#pragma once

#include "ui/core/Color.hpp"
#include "ui/core/Rect.hpp"
#include "ui/core/TextureRef.hpp"
#include "ui/core/Transform2D.hpp"

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace cinder::text {
class GlyphAtlas;
struct ShapedText;
}

namespace cinder::ui {

struct BoxStyle {
    Color fill = Color::transparent();
    Color border = Color::transparent();
    float borderWidth = 0.0f;
    glm::vec4 radii{0.0f};
};

struct BoxElement {
    Rect rect;
    BoxStyle style;
};

struct ImageElement {
    Rect rect;
    TextureRef texture;
    Color tint = Color::white();
    glm::vec2 uvMin{0.0f};
    glm::vec2 uvMax{1.0f};
    bool opaque = false;
};

struct GlyphQuad {
    int page = 0;
    Rect rect;
    glm::vec2 uvMin{0.0f};
    glm::vec2 uvMax{0.0f};
};

struct TextElement {
    std::vector<GlyphQuad> glyphs;
    Color color = Color::white();
};

struct LinesElement {
    std::vector<glm::vec2> points;
    Color color = Color::white();
    float thickness = 1.0f;
    bool closed = false;
};

struct PolygonElement {
    std::vector<glm::vec2> points;
    Color color = Color::white();
};

struct Element {
    int layer = 0;
    Rect clip;
    Transform2D transform;
    std::variant<BoxElement, ImageElement, TextElement, LinesElement, PolygonElement> shape;
};

class ElementList {
public:
    void reset(glm::vec2 size, float scale);

    glm::vec2 size() const { return size_; }
    float scale() const { return scale_; }
    Rect bounds() const { return {glm::vec2(0.0f), size_}; }

    void pushClip(const Rect& rect);
    void popClip();
    const Rect& clip() const { return clips_.back(); }

    void pushTransform(const Transform2D& transform);
    void popTransform();
    const Transform2D& transform() const { return transforms_.back(); }

    void box(int layer, const Rect& rect, const BoxStyle& style);
    void image(int layer, const Rect& rect, TextureRef texture, Color tint = Color::white(),
               glm::vec2 uvMin = glm::vec2(0.0f), glm::vec2 uvMax = glm::vec2(1.0f), bool opaque = false);
    void text(int layer, glm::vec2 baseline, const cinder::text::ShapedText& shaped, Color color,
              cinder::text::GlyphAtlas& atlas);
    void lines(int layer, std::span<const glm::vec2> points, Color color, float thickness,
               bool closed = false);
    void polygon(int layer, std::span<const glm::vec2> points, Color color);

    TextureRef named(std::string_view path);
    const std::vector<std::string>& names() const { return names_; }

    cinder::text::GlyphAtlas* atlas() const { return atlas_; }
    const std::vector<Element>& elements() const { return elements_; }
    bool empty() const { return elements_.empty(); }

private:
    Element& add(int layer);

    glm::vec2 size_{0.0f};
    float scale_ = 1.0f;
    std::vector<Rect> clips_{Rect{}};
    std::vector<Transform2D> transforms_{Transform2D{}};
    std::vector<Element> elements_;
    std::vector<std::string> names_;
    cinder::text::GlyphAtlas* atlas_ = nullptr;
};

}
