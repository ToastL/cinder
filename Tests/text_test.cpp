#include <doctest/doctest.h>

#include "Platform/Assets.hpp"
#include "Platform/Files.hpp"
#include "Text/Font.hpp"
#include "Text/FontSet.hpp"
#include "Text/GlyphAtlas.hpp"
#include "Text/Shaper.hpp"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <vector>

using cinder::Text::AtlasGlyph;
using cinder::Text::AtlasRect;
using cinder::Text::Font;
using cinder::Text::FontSet;
using cinder::Text::FontStyle;
using cinder::Text::GlyphAtlas;
using cinder::Text::ShapedText;
using cinder::Text::shape;

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

    const cinder::Text::FontMetrics metrics = regular().metrics(16.0f);
    CHECK(metrics.ascent > 10.0f);
    CHECK(metrics.descent < 0.0f);
    CHECK(metrics.lineHeight >= metrics.ascent - metrics.descent);
    CHECK(regular().metrics(32.0f).ascent == doctest::Approx(metrics.ascent * 2.0f));
}

TEST_CASE("a missing font file is an error that names the path") {
    CHECK_THROWS_WITH_AS(Font::load(cinder::Platform::enginePath("Content/Fonts/Missing.ttf")),
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
    const cinder::Text::GlyphBitmap bitmap = regular().rasterize(regular().glyphIndex(U'A'), 24.0f);
    CHECK(bitmap.width > 5);
    CHECK(bitmap.height > 10);
    CHECK(bitmap.bearing.y < 0.0f);
    CHECK(*std::max_element(bitmap.coverage.begin(), bitmap.coverage.end()) == 255);
}

TEST_CASE("shaping maps one glyph per character, so fi is not a ligature") {
    const ShapedText shaped = shape(regular(), "fi", 16.0f);
    REQUIRE(shaped.glyphs.size() == 2);
    CHECK(shaped.glyphs[0].glyph == regular().glyphIndex(U'f'));
    CHECK(shaped.glyphs[1].glyph == regular().glyphIndex(U'i'));
}

TEST_CASE("character lookup covers both cmap formats and misses map to glyph zero") {
    const Font& mono = fonts().get(FontStyle::Mono);
    CHECK(regular().glyphIndex(U'A') != 0);
    CHECK(mono.glyphIndex(U'A') != 0);
    CHECK(regular().glyphIndex(U'A') != regular().glyphIndex(U'B'));
    CHECK(regular().glyphIndex(U'\U0001F600') == 0);
    CHECK(mono.glyphIndex(U'\U0001F600') == 0);
    CHECK(mono.glyphIndex(U'￿') == 0);
}

TEST_CASE("a font without GPOS kerning kerns nothing") {
    const Font& mono = fonts().get(FontStyle::Mono);
    CHECK(mono.kerning(mono.glyphIndex(U'A'), mono.glyphIndex(U'V')) == 0.0f);
    CHECK(regular().kerning(regular().glyphIndex(U'A'), regular().glyphIndex(U'V')) < 0.0f);
}

TEST_CASE("a composite glyph draws its accent above the base letter") {
    const cinder::Text::GlyphBitmap e = regular().rasterize(regular().glyphIndex(U'e'), 32.0f);
    const cinder::Text::GlyphBitmap acute = regular().rasterize(regular().glyphIndex(U'é'), 32.0f);
    CHECK(acute.height > e.height + 3);
    CHECK(acute.bearing.y < e.bearing.y - 3.0f);
    CHECK(acute.bearing.y + static_cast<float>(acute.height) == doctest::Approx(e.bearing.y + static_cast<float>(e.height)));
}

TEST_CASE("a counter stays open, so O is empty in the middle and solid on its ring") {
    const cinder::Text::GlyphBitmap o = regular().rasterize(regular().glyphIndex(U'O'), 48.0f);
    REQUIRE(o.width > 20);
    const auto at = [&](int x, int y) { return o.coverage[static_cast<std::size_t>(y * o.width + x)]; };
    CHECK(at(o.width / 2, o.height / 2) == 0);
    CHECK(at(1, o.height / 2) == 255);
    CHECK(at(o.width / 2, 1) == 255);
}

TEST_CASE("truncated font bytes are rejected as malformed") {
    const std::vector<std::uint8_t> whole = cinder::Platform::readBinaryFile(
        cinder::Platform::enginePath("Content/Fonts/Roboto-Regular.ttf"));
    CHECK_THROWS_AS(Font(std::vector<std::uint8_t>(whole.begin(), whole.begin() + 100)), std::runtime_error);
    CHECK_THROWS_AS(Font(std::vector<std::uint8_t>{}), std::runtime_error);
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
