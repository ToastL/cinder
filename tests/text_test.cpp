#include <doctest/doctest.h>

#include "platform/Assets.hpp"
#include "text/Font.hpp"
#include "text/FontSet.hpp"
#include "text/GlyphAtlas.hpp"
#include "text/Shaper.hpp"

#include <algorithm>
#include <cstdint>
#include <optional>

using cinder::text::AtlasGlyph;
using cinder::text::AtlasRect;
using cinder::text::Font;
using cinder::text::FontSet;
using cinder::text::FontStyle;
using cinder::text::GlyphAtlas;
using cinder::text::ShapedText;
using cinder::text::shape;

namespace {

const FontSet& fonts() {
    static const FontSet set = FontSet::engineDefault();
    return set;
}

const Font& regular() { return fonts().get(FontStyle::Regular); }

}

TEST_CASE("the engine's fonts load and report sensible metrics") {
    CHECK(regular().family() == "Roboto");
    CHECK(fonts().get(FontStyle::Bold).family() == "Roboto");
    CHECK(fonts().get(FontStyle::Mono).family() == "Roboto Mono");

    const cinder::text::FontMetrics metrics = regular().metrics(16.0f);
    CHECK(metrics.ascent > 10.0f);
    CHECK(metrics.descent < 0.0f);
    CHECK(metrics.lineHeight >= metrics.ascent - metrics.descent);
    CHECK(regular().metrics(32.0f).ascent == doctest::Approx(metrics.ascent * 2.0f));
}

TEST_CASE("a missing font file is an error that names the path") {
    CHECK_THROWS_WITH_AS(Font::load(cinder::platform::enginePath("fonts/Missing.ttf")),
                         doctest::Contains("Missing.ttf"), std::exception);
}

TEST_CASE("shaping reads GPOS kerning, so AV is narrower than A and V apart") {
    const float apart = shape(regular(), "A", 32.0f).width + shape(regular(), "V", 32.0f).width;
    const ShapedText together = shape(regular(), "AV", 32.0f);
    REQUIRE(together.glyphs.size() == 2);
    CHECK(together.width < apart - 0.5f);
    CHECK(together.width == doctest::Approx(together.glyphs[0].advance + together.glyphs[1].advance));
}

TEST_CASE("clusters are byte offsets into the UTF-8 string") {
    const ShapedText shaped = shape(regular(), "a\xC3\xA9 b", 16.0f);
    REQUIRE(shaped.glyphs.size() == 4);
    CHECK(shaped.glyphs[0].cluster == 0);
    CHECK(shaped.glyphs[1].cluster == 1);
    CHECK(shaped.glyphs[2].cluster == 3);
    CHECK(shaped.glyphs[3].cluster == 4);
    CHECK(shaped.glyphs[1].glyph == regular().glyphIndex(U'é'));
    CHECK(shape(regular(), "", 16.0f).glyphs.empty());
}

TEST_CASE("the mono font advances every character by the same width, and width scales with size") {
    const Font& mono = fonts().get(FontStyle::Mono);
    const ShapedText shaped = shape(mono, "il WM.", 14.0f);
    REQUIRE(shaped.glyphs.size() == 6);
    for (const auto& glyph : shaped.glyphs) CHECK(glyph.advance == doctest::Approx(shaped.glyphs[0].advance));
    CHECK(shape(mono, "il WM.", 28.0f).width == doctest::Approx(shaped.width * 2.0f));
}

TEST_CASE("a rasterized glyph has coverage and a bearing above the baseline") {
    const cinder::text::GlyphBitmap bitmap = regular().rasterize(regular().glyphIndex(U'A'), 24.0f);
    CHECK(bitmap.width > 5);
    CHECK(bitmap.height > 10);
    CHECK(bitmap.bearing.y < 0.0f);
    CHECK(*std::max_element(bitmap.coverage.begin(), bitmap.coverage.end()) == 255);
}

TEST_CASE("the atlas caches a glyph per size, marks what it wrote, and skips blank glyphs") {
    GlyphAtlas atlas;
    const std::uint32_t a = regular().glyphIndex(U'A');

    const AtlasGlyph& first = atlas.glyph(regular(), a, 16.0f);
    REQUIRE_FALSE(first.empty());
    CHECK(atlas.pageCount() == 1);
    CHECK(first.rect.x >= GlyphAtlas::PADDING);
    CHECK(first.rect.y >= GlyphAtlas::PADDING);

    const AtlasGlyph& again = atlas.glyph(regular(), a, 16.0f);
    CHECK(&again == &first);
    CHECK(atlas.size() == 1);

    atlas.clearDirty(0);
    CHECK_FALSE(atlas.dirty(0));
    const AtlasGlyph& larger = atlas.glyph(regular(), a, 32.0f);
    CHECK(atlas.size() == 2);
    CHECK(larger.rect.height > first.rect.height);
    const std::optional<AtlasRect> dirty = atlas.dirty(0);
    REQUIRE(dirty);
    CHECK(*dirty == larger.rect);

    const AtlasGlyph& space = atlas.glyph(regular(), regular().glyphIndex(U' '), 16.0f);
    CHECK(space.empty());
    CHECK(atlas.size() == 3);

    const auto& pixels = atlas.pixels(0);
    const std::size_t row = static_cast<std::size_t>(larger.rect.y + larger.rect.height / 2) * GlyphAtlas::PAGE_SIZE;
    CHECK(std::any_of(pixels.begin() + static_cast<std::ptrdiff_t>(row + larger.rect.x),
                      pixels.begin() + static_cast<std::ptrdiff_t>(row + larger.rect.x + larger.rect.width),
                      [](std::uint8_t value) { return value > 0; }));
}

TEST_CASE("clearing the atlas drops every glyph and dirties every page for a rescale") {
    GlyphAtlas atlas;
    atlas.glyph(regular(), regular().glyphIndex(U'B'), 20.0f);
    atlas.clearDirty(0);
    atlas.clear();
    CHECK(atlas.size() == 0);
    CHECK(atlas.pageCount() == 1);
    REQUIRE(atlas.dirty(0));
    CHECK(atlas.dirty(0)->width == GlyphAtlas::PAGE_SIZE);
}

TEST_CASE("a full page spills into a second one") {
    GlyphAtlas atlas;
    for (char32_t c = U'!'; c <= U'~'; ++c) atlas.glyph(regular(), regular().glyphIndex(c), 250.0f);
    CHECK(atlas.pageCount() >= 2);
}
