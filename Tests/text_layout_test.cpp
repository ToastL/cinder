#include <doctest/doctest.h>

#include "text/FontSet.hpp"
#include "text/Shaper.hpp"
#include "text/TextLayout.hpp"
#include "text/Utf8.hpp"

#include <string>

using namespace cinder::text;

namespace {

const FontSet& fonts() {
    static const FontSet set = FontSet::engineDefault();
    return set;
}

}

TEST_CASE("boundaries step over whole UTF-8 characters") {
    const std::string text = "a\xC3\xA9\xE2\x82\xAC\xF0\x9F\x98\x80";
    CHECK(nextBoundary(text, 0) == 1);
    CHECK(nextBoundary(text, 1) == 3);
    CHECK(nextBoundary(text, 3) == 6);
    CHECK(nextBoundary(text, 6) == 10);
    CHECK(nextBoundary(text, 10) == 10);
    CHECK(previousBoundary(text, 10) == 6);
    CHECK(previousBoundary(text, 6) == 3);
    CHECK(previousBoundary(text, 1) == 0);
    CHECK(previousBoundary(text, 0) == 0);
    CHECK(characterCount(text) == 4);
    CHECK(decodeAt(text, 6) == U'\U0001F600');
}

TEST_CASE("encoding and decoding round trip every width") {
    for (const char32_t codepoint : {U'A', U'é', U'€', U'\U0001F600'}) {
        std::string text;
        appendUtf8(text, codepoint);
        CHECK(decodeAt(text, 0) == codepoint);
        CHECK(nextBoundary(text, 0) == text.size());
    }
}

TEST_CASE("word jumps skip punctuation and land on word edges") {
    const std::string text = "hello, world_x  y";
    CHECK(nextWord(text, 0) == 5);
    CHECK(nextWord(text, 5) == 14);
    CHECK(nextWord(text, 14) == 17);
    CHECK(previousWord(text, 17) == 16);
    CHECK(previousWord(text, 16) == 7);
    CHECK(previousWord(text, 7) == 0);
}

TEST_CASE("caret stops sit on every character boundary, left to right") {
    const std::string text = "Wa\xC3\xA9";
    const ShapedText shaped = shape(fonts().get(FontStyle::Regular), text, 16.0f);
    const CaretStops stops = caretStops(shaped, text);
    REQUIRE(stops.offsets.size() == 4);
    CHECK(stops.offsets[2] == 2);
    CHECK(stops.offsets[3] == 4);
    CHECK(stops.positions[0] == 0.0f);
    CHECK(stops.positions[3] == doctest::Approx(shaped.width));
    for (std::size_t i = 1; i < stops.positions.size(); ++i) CHECK(stops.positions[i] > stops.positions[i - 1]);

    CHECK(stops.nearest(-5.0f) == 0);
    CHECK(stops.nearest(1000.0f) == 4);
    CHECK(stops.nearest(stops.positions[2] + 0.4f) == 2);
    CHECK(stops.positionOf(2) == stops.positions[2]);

    const CaretStops empty = caretStops(shape(fonts().get(FontStyle::Regular), "", 16.0f), "");
    REQUIRE(empty.offsets.size() == 1);
    CHECK(empty.positions[0] == 0.0f);
}
