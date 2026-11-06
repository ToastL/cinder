#include "ui/core/Batcher.hpp"

#include "ui/core/ElementList.hpp"

#include <glm/common.hpp>
#include <glm/geometric.hpp>

#include <algorithm>
#include <cstddef>
#include <numeric>
#include <type_traits>

namespace cinder::ui {

namespace {

constexpr float MITER_LIMIT = 100.0f;
constexpr float DEGENERATE = 1e-6f;

class Builder {
public:
    Builder(UiGeometry& out, float scale) : out_(out), scale_(scale), feather_(1.0f / scale) {}

    void use(TextureRef texture, const Rect& clip) {
        if (out_.batches.empty() || out_.batches.back().clip != clip) {
            open(texture, clip);
            return;
        }
        if (texture.kind == TextureRef::Kind::None) return;
        UiBatch& current = out_.batches.back();
        if (current.texture.kind == TextureRef::Kind::None) {
            current.texture = texture;
        } else if (current.texture != texture) {
            open(texture, clip);
        }
    }

    void finish() {
        for (std::size_t i = 0; i < out_.batches.size(); ++i) {
            const std::uint32_t end = i + 1 < out_.batches.size()
                    ? out_.batches[i + 1].firstIndex
                    : static_cast<std::uint32_t>(out_.indices.size());
            out_.batches[i].indexCount = end - out_.batches[i].firstIndex;
        }
        std::erase_if(out_.batches, [](const UiBatch& batch) { return batch.indexCount == 0; });
    }

    void box(const Element& element, const BoxElement& box) {
        use(TextureRef::none(), element.clip);
        const Rect rect = snap(element, box.rect);
        const glm::vec2 half = rect.size() * 0.5f;
        const float limit = std::min(half.x, half.y);
        const glm::vec4 radii = glm::clamp(box.style.radii, glm::vec4(0.0f), glm::vec4(limit));
        const glm::vec4 shape(half, std::min(box.style.borderWidth, limit), static_cast<float>(UiMode::Box));

        const Rect outer = rect.inset(-feather_);
        const glm::vec2 centre = rect.center();
        UiVertex vertex;
        vertex.color = box.style.fill.vec();
        vertex.border = box.style.border.vec();
        vertex.shape = shape;
        vertex.radii = radii;
        quad(element, outer, vertex, [&](glm::vec2 corner, glm::vec2) { return corner - centre; });
    }

    void image(const Element& element, const ImageElement& image) {
        use(image.texture, element.clip);
        const Rect rect = snap(element, image.rect);
        UiVertex vertex;
        vertex.color = image.tint.vec();
        vertex.shape.w = static_cast<float>(image.opaque ? UiMode::Opaque : UiMode::Textured);
        quad(element, rect, vertex, [&](glm::vec2, glm::vec2 unit) {
            return glm::mix(image.uvMin, image.uvMax, unit);
        });
    }

    void text(const Element& element, const TextElement& text) {
        UiVertex vertex;
        vertex.color = text.color.vec();
        vertex.shape.w = static_cast<float>(UiMode::Glyph);
        for (const GlyphQuad& glyph : text.glyphs) {
            use(TextureRef::glyphPage(glyph.page), element.clip);
            quad(element, glyph.rect, vertex, [&](glm::vec2, glm::vec2 unit) {
                return glm::mix(glyph.uvMin, glyph.uvMax, unit);
            });
        }
    }

    void lines(const Element& element, const LinesElement& lines) {
        use(TextureRef::none(), element.clip);
        std::vector<glm::vec2> points;
        points.reserve(lines.points.size());
        for (const glm::vec2& point : lines.points) points.push_back(element.transform.apply(point));

        const std::size_t count = points.size();
        const std::size_t segments = lines.closed ? count : count - 1;
        std::vector<glm::vec2> normals(segments);
        for (std::size_t i = 0; i < segments; ++i) {
            const glm::vec2 delta = points[(i + 1) % count] - points[i];
            const float length = glm::length(delta);
            normals[i] = length > DEGENERATE ? glm::vec2(-delta.y, delta.x) / length : glm::vec2(0.0f);
        }

        const bool thick = lines.thickness > feather_;
        const float half = thick ? (lines.thickness - feather_) * 0.5f : 0.0f;
        const glm::vec4 solid = lines.color.vec();
        const glm::vec4 faded(solid.r, solid.g, solid.b, solid.a * std::min(1.0f, lines.thickness / feather_));
        const glm::vec4 clear(solid.r, solid.g, solid.b, 0.0f);
        const std::uint32_t across = thick ? 4 : 3;
        const auto base = static_cast<std::uint32_t>(out_.vertices.size());

        for (std::size_t i = 0; i < count; ++i) {
            const glm::vec2 normal = jointNormal(normals, i, count, lines.closed);
            if (thick) {
                push(points[i] + normal * (half + feather_), clear);
                push(points[i] + normal * half, solid);
                push(points[i] - normal * half, solid);
                push(points[i] - normal * (half + feather_), clear);
            } else {
                push(points[i] + normal * feather_, clear);
                push(points[i], faded);
                push(points[i] - normal * feather_, clear);
            }
        }

        for (std::size_t i = 0; i < segments; ++i) {
            const std::uint32_t from = base + static_cast<std::uint32_t>(i) * across;
            const std::uint32_t to = base + static_cast<std::uint32_t>((i + 1) % count) * across;
            for (std::uint32_t strip = 0; strip + 1 < across; ++strip) {
                indices(from + strip, to + strip, to + strip + 1);
                indices(from + strip, to + strip + 1, from + strip + 1);
            }
        }
    }

    void polygon(const Element& element, const PolygonElement& polygon) {
        use(TextureRef::none(), element.clip);
        std::vector<glm::vec2> points;
        points.reserve(polygon.points.size());
        for (const glm::vec2& point : polygon.points) points.push_back(element.transform.apply(point));

        const std::size_t count = points.size();
        float area = 0.0f;
        for (std::size_t i = 0; i < count; ++i) {
            const glm::vec2& a = points[i];
            const glm::vec2& b = points[(i + 1) % count];
            area += a.x * b.y - b.x * a.y;
        }
        const float side = area >= 0.0f ? 1.0f : -1.0f;

        std::vector<glm::vec2> normals(count);
        for (std::size_t i = 0; i < count; ++i) {
            const glm::vec2 delta = points[(i + 1) % count] - points[i];
            const float length = glm::length(delta);
            normals[i] = length > DEGENERATE ? side * glm::vec2(delta.y, -delta.x) / length : glm::vec2(0.0f);
        }

        const glm::vec4 solid = polygon.color.vec();
        const glm::vec4 clear(solid.r, solid.g, solid.b, 0.0f);
        const auto base = static_cast<std::uint32_t>(out_.vertices.size());
        for (std::size_t i = 0; i < count; ++i) {
            const glm::vec2 normal = jointNormal(normals, i, count, true) * (feather_ * 0.5f);
            push(points[i] - normal, solid);
            push(points[i] + normal, clear);
        }

        for (std::uint32_t i = 1; i + 1 < count; ++i) indices(base, base + i * 2, base + (i + 1) * 2);
        for (std::uint32_t i = 0; i < count; ++i) {
            const std::uint32_t from = base + i * 2;
            const std::uint32_t to = base + static_cast<std::uint32_t>((i + 1) % count) * 2;
            indices(from, to, to + 1);
            indices(from, to + 1, from + 1);
        }
    }

    void mesh(const Element& element, const MeshElement& mesh) {
        use(TextureRef::none(), element.clip);
        const auto base = static_cast<std::uint32_t>(out_.vertices.size());
        const auto count = static_cast<std::uint32_t>(mesh.positions.size());
        for (std::size_t i = 0; i < mesh.positions.size(); ++i) {
            push(element.transform.apply(mesh.positions[i]), mesh.colors[i].vec());
        }
        for (std::size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
            if (mesh.indices[i] >= count || mesh.indices[i + 1] >= count || mesh.indices[i + 2] >= count) continue;
            indices(base + mesh.indices[i], base + mesh.indices[i + 1], base + mesh.indices[i + 2]);
        }
    }

private:
    static glm::vec2 jointNormal(const std::vector<glm::vec2>& normals, std::size_t point, std::size_t count,
                                 bool closed) {
        const std::size_t segments = normals.size();
        glm::vec2 before;
        glm::vec2 after;
        if (closed) {
            before = normals[(point + segments - 1) % segments];
            after = normals[point % segments];
        } else {
            before = normals[point == 0 ? 0 : point - 1];
            after = normals[point + 1 == count ? segments - 1 : point];
        }
        glm::vec2 average = (before + after) * 0.5f;
        const float lengthSquared = glm::dot(average, average);
        if (lengthSquared > DEGENERATE) average *= std::min(1.0f / lengthSquared, MITER_LIMIT);
        return average;
    }

    Rect snap(const Element& element, const Rect& rect) const {
        if (element.transform.linear != glm::mat2(1.0f)) return rect;
        const glm::vec2 offset = element.transform.translation;
        return {glm::round((rect.min + offset) * scale_) / scale_ - offset,
                glm::round((rect.max + offset) * scale_) / scale_ - offset};
    }

    template <typename Uv>
    void quad(const Element& element, const Rect& rect, UiVertex vertex, Uv uv) {
        static constexpr glm::vec2 UNITS[4] = {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};
        const auto base = static_cast<std::uint32_t>(out_.vertices.size());
        for (const glm::vec2& unit : UNITS) {
            const glm::vec2 corner = glm::mix(rect.min, rect.max, unit);
            vertex.position = element.transform.apply(corner);
            vertex.uv = uv(corner, unit);
            out_.vertices.push_back(vertex);
        }
        indices(base, base + 1, base + 2);
        indices(base, base + 2, base + 3);
    }

    void push(glm::vec2 position, const glm::vec4& color) {
        UiVertex vertex;
        vertex.position = position;
        vertex.color = color;
        vertex.shape.w = static_cast<float>(UiMode::Solid);
        out_.vertices.push_back(vertex);
    }

    void indices(std::uint32_t a, std::uint32_t b, std::uint32_t c) {
        out_.indices.push_back(a);
        out_.indices.push_back(b);
        out_.indices.push_back(c);
    }

    void open(TextureRef texture, const Rect& clip) {
        out_.batches.push_back(UiBatch{texture, clip, static_cast<std::uint32_t>(out_.indices.size()), 0});
    }

    UiGeometry& out_;
    float scale_;
    float feather_;
};

}

void UiGeometry::clear() {
    vertices.clear();
    indices.clear();
    batches.clear();
}

void batch(const ElementList& list, UiGeometry& out) {
    out.clear();
    const std::vector<Element>& elements = list.elements();
    std::vector<std::size_t> order(elements.size());
    std::iota(order.begin(), order.end(), std::size_t{0});
    std::stable_sort(order.begin(), order.end(),
                     [&](std::size_t a, std::size_t b) { return elements[a].layer < elements[b].layer; });

    Builder builder(out, list.scale());
    for (const std::size_t index : order) {
        const Element& element = elements[index];
        if (element.clip.empty()) continue;
        std::visit(
                [&](const auto& shape) {
                    using Shape = std::decay_t<decltype(shape)>;
                    if constexpr (std::is_same_v<Shape, BoxElement>) builder.box(element, shape);
                    else if constexpr (std::is_same_v<Shape, ImageElement>) builder.image(element, shape);
                    else if constexpr (std::is_same_v<Shape, TextElement>) builder.text(element, shape);
                    else if constexpr (std::is_same_v<Shape, LinesElement>) builder.lines(element, shape);
                    else if constexpr (std::is_same_v<Shape, PolygonElement>) builder.polygon(element, shape);
                    else builder.mesh(element, shape);
                },
                element.shape);
    }
    builder.finish();
}

}
