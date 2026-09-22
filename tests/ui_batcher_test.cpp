#include <doctest/doctest.h>

#include "text/FontSet.hpp"
#include "text/GlyphAtlas.hpp"
#include "text/Shaper.hpp"
#include "ui/core/Batcher.hpp"
#include "ui/core/ElementList.hpp"

#include <array>

using cinder::ui::BoxStyle;
using cinder::ui::ElementList;
using cinder::ui::LinearColor;
using cinder::ui::Rect;
using cinder::ui::TextureRef;
using cinder::ui::Transform2D;
using cinder::ui::UiGeometry;
using cinder::ui::UiMode;

namespace {

const glm::vec2 SIZE(400.0f, 300.0f);
const BoxStyle GREY{LinearColor::hex(0x808080FF)};

ElementList list(float scale = 1.0f) {
    ElementList elements;
    elements.reset(SIZE, scale);
    return elements;
}

UiGeometry batched(const ElementList& elements) {
    UiGeometry geometry;
    cinder::ui::batch(elements, geometry);
    return geometry;
}

const cinder::text::FontSet& fonts() {
    static const cinder::text::FontSet set = cinder::text::FontSet::engineDefault();
    return set;
}

UiMode modeOf(const cinder::ui::UiVertex& vertex) { return static_cast<UiMode>(static_cast<int>(vertex.shape.w)); }

}

TEST_CASE("the clip stack intersects on push and never pops its root") {
    ElementList elements = list();
    CHECK(elements.clip() == elements.bounds());
    elements.pushClip(Rect::fromSize({-10.0f, 20.0f}, {100.0f, 500.0f}));
    CHECK(elements.clip() == Rect{{0.0f, 20.0f}, {90.0f, 300.0f}});
    elements.pushClip(Rect::fromSize({500.0f, 500.0f}, {10.0f, 10.0f}));
    CHECK(elements.clip().empty());
    elements.popClip();
    elements.popClip();
    elements.popClip();
    CHECK(elements.clip() == elements.bounds());
}

TEST_CASE("boxes, lines and text share one batch because only the text samples a texture") {
    cinder::text::GlyphAtlas atlas;
    ElementList elements = list();
    elements.box(0, Rect::fromSize({10.0f, 10.0f}, {50.0f, 20.0f}), GREY);
    elements.text(0, {12.0f, 25.0f}, cinder::text::shape(fonts().get(cinder::text::FontStyle::Regular), "Hi", 13.0f),
                  LinearColor::white(), atlas);
    const std::array<glm::vec2, 2> line = {glm::vec2(0.0f), glm::vec2(100.0f, 0.0f)};
    elements.lines(0, line, LinearColor::white(), 2.0f);
    elements.box(0, Rect::fromSize({70.0f, 10.0f}, {50.0f, 20.0f}), GREY);

    const UiGeometry geometry = batched(elements);
    REQUIRE(geometry.batches.size() == 1);
    CHECK(geometry.batches[0].texture == TextureRef::glyphPage(0));
    CHECK(geometry.batches[0].indexCount == geometry.indices.size());
    CHECK(elements.atlas() == &atlas);
}

TEST_CASE("a different texture or clip starts a new batch") {
    ElementList elements = list();
    const Rect rect = Rect::fromSize({10.0f, 10.0f}, {20.0f, 20.0f});
    elements.image(0, rect, TextureRef::viewport());
    elements.image(0, rect, elements.named("a.png"));
    elements.image(0, rect, elements.named("a.png"));
    elements.pushClip(Rect::fromSize({0.0f, 0.0f}, {100.0f, 100.0f}));
    elements.box(0, rect, GREY);
    elements.popClip();

    const UiGeometry geometry = batched(elements);
    REQUIRE(geometry.batches.size() == 3);
    CHECK(geometry.batches[0].texture == TextureRef::viewport());
    CHECK(geometry.batches[1].texture == TextureRef{TextureRef::Kind::Named, 0});
    CHECK(geometry.batches[1].indexCount == 12);
    CHECK(geometry.batches[2].clip == Rect::fromSize({0.0f, 0.0f}, {100.0f, 100.0f}));
    CHECK(elements.names().size() == 1);
}

TEST_CASE("layers order the output and keep submission order within a layer") {
    ElementList elements = list();
    elements.box(5, Rect::fromSize({100.0f, 0.0f}, {10.0f, 10.0f}), GREY);
    elements.box(1, Rect::fromSize({200.0f, 0.0f}, {10.0f, 10.0f}), GREY);
    elements.box(1, Rect::fromSize({300.0f, 0.0f}, {10.0f, 10.0f}), GREY);

    const UiGeometry geometry = batched(elements);
    REQUIRE(geometry.vertices.size() == 12);
    CHECK(geometry.vertices[0].position.x < 201.0f);
    CHECK(geometry.vertices[4].position.x > 298.0f);
    CHECK(geometry.vertices[8].position.x < 101.0f);
}

TEST_CASE("a box is a feathered quad carrying its local position, snapped to device pixels") {
    ElementList elements = list(2.0f);
    elements.box(0, Rect::fromSize({10.3f, 20.0f}, {40.0f, 10.0f}),
                 BoxStyle{LinearColor::white(), LinearColor::black(), 1.0f, glm::vec4(3.0f, 30.0f, 0.0f, -2.0f)});

    const UiGeometry geometry = batched(elements);
    REQUIRE(geometry.vertices.size() == 4);
    const auto& corner = geometry.vertices[0];
    CHECK(modeOf(corner) == UiMode::Box);
    CHECK(corner.position.x == doctest::Approx(10.0f));
    CHECK(corner.position.y == doctest::Approx(19.5f));
    CHECK(corner.uv.x == doctest::Approx(-20.5f));
    CHECK(corner.shape.x == doctest::Approx(20.0f));
    CHECK(corner.shape.y == doctest::Approx(5.0f));
    CHECK(corner.radii == glm::vec4(3.0f, 5.0f, 0.0f, 0.0f));
    CHECK(geometry.indices.size() == 6);
}

TEST_CASE("thick lines get a solid core and a fringe, thin ones a faded spine") {
    const std::array<glm::vec2, 2> segment = {glm::vec2(10.0f, 10.0f), glm::vec2(110.0f, 10.0f)};

    ElementList thick = list();
    thick.lines(0, segment, LinearColor::white(), 3.0f);
    const UiGeometry wide = batched(thick);
    CHECK(wide.vertices.size() == 8);
    CHECK(wide.indices.size() == 18);
    CHECK(wide.vertices[0].color.a == 0.0f);
    CHECK(wide.vertices[1].color.a == 1.0f);
    CHECK(wide.vertices[0].position.y - wide.vertices[3].position.y == doctest::Approx(4.0f));

    ElementList thin = list();
    thin.lines(0, segment, LinearColor::white(), 0.5f);
    const UiGeometry narrow = batched(thin);
    CHECK(narrow.vertices.size() == 6);
    CHECK(narrow.indices.size() == 12);
    CHECK(narrow.vertices[1].color.a == doctest::Approx(0.5f));
}

TEST_CASE("a convex polygon is a fan plus a fringe, whichever way it winds") {
    const std::array<glm::vec2, 3> clockwise = {glm::vec2(0.0f), glm::vec2(10.0f, 0.0f), glm::vec2(0.0f, 10.0f)};
    const std::array<glm::vec2, 3> anticlockwise = {glm::vec2(0.0f), glm::vec2(0.0f, 10.0f), glm::vec2(10.0f, 0.0f)};

    for (const auto& points : {clockwise, anticlockwise}) {
        ElementList elements = list();
        elements.polygon(0, points, LinearColor::white());
        const UiGeometry geometry = batched(elements);
        REQUIRE(geometry.vertices.size() == 6);
        CHECK(geometry.indices.size() == 21);
        const glm::vec2 inner = geometry.vertices[0].position;
        const glm::vec2 outer = geometry.vertices[1].position;
        CHECK(outer.x < inner.x);
        CHECK(outer.y < inner.y);
    }
}

TEST_CASE("a transform moves every vertex, and a clipped-away element emits nothing") {
    ElementList elements = list();
    elements.pushTransform(Transform2D::translate({100.0f, 50.0f}));
    elements.image(0, Rect::fromSize({0.0f, 0.0f}, {10.0f, 10.0f}), TextureRef::viewport());
    elements.popTransform();
    elements.pushClip(Rect::fromSize({1000.0f, 1000.0f}, {10.0f, 10.0f}));
    elements.box(0, Rect::fromSize({0.0f, 0.0f}, {10.0f, 10.0f}), GREY);
    elements.popClip();

    const UiGeometry geometry = batched(elements);
    REQUIRE(geometry.vertices.size() == 4);
    CHECK(geometry.vertices[0].position == glm::vec2(100.0f, 50.0f));
    CHECK(geometry.vertices[2].uv == glm::vec2(1.0f));
    CHECK(modeOf(geometry.vertices[0]) == UiMode::Textured);
}

TEST_CASE("glyph quads land on device pixels and point at their atlas texels") {
    cinder::text::GlyphAtlas atlas;
    ElementList elements = list(2.0f);
    elements.text(0, {10.3f, 20.1f}, cinder::text::shape(fonts().get(cinder::text::FontStyle::Regular), "A", 13.0f),
                  LinearColor::white(), atlas);

    const UiGeometry geometry = batched(elements);
    REQUIRE(geometry.vertices.size() == 4);
    const glm::vec2 corner = geometry.vertices[0].position * 2.0f;
    CHECK(corner.x == doctest::Approx(std::round(corner.x)));
    CHECK(corner.y == doctest::Approx(std::round(corner.y)));
    CHECK(modeOf(geometry.vertices[0]) == UiMode::Glyph);
    const glm::vec2 texels = (geometry.vertices[2].uv - geometry.vertices[0].uv)
            * static_cast<float>(cinder::text::GlyphAtlas::PAGE_SIZE);
    const glm::vec2 pixels = (geometry.vertices[2].position - geometry.vertices[0].position) * 2.0f;
    CHECK(texels.x == doctest::Approx(pixels.x));
    CHECK(texels.y == doctest::Approx(pixels.y));
}
