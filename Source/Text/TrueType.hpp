#pragma once

#include <glm/mat2x2.hpp>
#include <glm/vec2.hpp>

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace cinder::Text {

struct OutlineSegment {
    glm::vec2 from{0.0f};
    glm::vec2 control{0.0f};
    glm::vec2 to{0.0f};
    bool curved = false;
};

struct Outline {
    std::vector<OutlineSegment> segments;
    glm::vec2 min{0.0f};
    glm::vec2 max{0.0f};

    bool empty() const { return segments.empty(); }
};

class TrueType {
public:
    explicit TrueType(std::span<const std::uint8_t> bytes);

    const std::string& family() const { return family_; }
    int unitsPerEm() const { return unitsPerEm_; }
    int ascender() const { return ascender_; }
    int descender() const { return descender_; }
    int lineGap() const { return lineGap_; }

    std::uint32_t glyphIndex(char32_t codepoint) const;
    int advance(std::uint32_t glyph) const;
    int kerning(std::uint32_t left, std::uint32_t right) const;
    Outline outline(std::uint32_t glyph) const;

private:
    struct Table {
        std::uint32_t offset = 0;
        std::uint32_t length = 0;

        bool present() const { return length > 0; }
    };

    Table table(const char* tag) const;
    std::uint8_t u8(std::size_t at) const;
    std::uint16_t u16(std::size_t at) const;
    std::int16_t i16(std::size_t at) const;
    std::uint32_t u32(std::size_t at) const;

    void readMetrics();
    void readCmap();
    void readFamily();
    void readKerning();

    std::uint32_t lookupFormat4(char32_t codepoint) const;
    std::uint32_t lookupFormat12(char32_t codepoint) const;
    int coverageIndex(std::size_t coverage, std::uint32_t glyph) const;
    int glyphClass(std::size_t classDef, std::uint32_t glyph) const;
    bool pairAdjustment(std::size_t subtable, std::uint32_t left, std::uint32_t right, int& adjustment) const;
    void appendGlyph(std::uint32_t glyph, const glm::mat2& transform, glm::vec2 offset, int depth,
                     std::vector<OutlineSegment>& segments) const;

    std::span<const std::uint8_t> bytes_;
    std::string family_;
    int unitsPerEm_ = 1000;
    int ascender_ = 0;
    int descender_ = 0;
    int lineGap_ = 0;
    int glyphCount_ = 0;
    int horizontalMetrics_ = 0;
    bool longLoca_ = false;
    Table hmtx_;
    Table loca_;
    Table glyf_;
    std::size_t cmap_ = 0;
    std::uint16_t cmapFormat_ = 0;
    std::vector<std::vector<std::size_t>> kernLookups_;
};

}
