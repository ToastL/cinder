#include "Text/TrueType.hpp"

#include "Platform/Utf8.hpp"

#include <glm/common.hpp>

#include <algorithm>
#include <bit>
#include <cstring>
#include <stdexcept>

namespace cinder::Text {

namespace {

constexpr int MAX_COMPOSITE_DEPTH = 8;
constexpr std::uint16_t NO_REQUIRED_FEATURE = 0xFFFF;

constexpr std::uint16_t ARG_1_AND_2_ARE_WORDS = 0x0001;
constexpr std::uint16_t ARGS_ARE_XY_VALUES = 0x0002;
constexpr std::uint16_t WE_HAVE_A_SCALE = 0x0008;
constexpr std::uint16_t MORE_COMPONENTS = 0x0020;
constexpr std::uint16_t X_AND_Y_SCALE = 0x0040;
constexpr std::uint16_t TWO_BY_TWO = 0x0080;

constexpr std::uint8_t ON_CURVE = 0x01;
constexpr std::uint8_t X_SHORT = 0x02;
constexpr std::uint8_t Y_SHORT = 0x04;
constexpr std::uint8_t REPEAT = 0x08;
constexpr std::uint8_t X_SAME_OR_POSITIVE = 0x10;
constexpr std::uint8_t Y_SAME_OR_POSITIVE = 0x20;

constexpr std::uint16_t VALUE_X_ADVANCE = 0x0004;

[[noreturn]] void malformed() { throw std::runtime_error("malformed font"); }

int valueSize(std::uint16_t format) { return 2 * std::popcount(static_cast<unsigned>(format & 0xFFu)); }

float f2dot14(std::int16_t value) { return static_cast<float>(value) / 16384.0f; }

struct ContourPoint {
    glm::vec2 position{0.0f};
    bool onCurve = false;
};

void appendContour(const std::vector<ContourPoint>& points, std::vector<OutlineSegment>& segments) {
    const std::size_t count = points.size();
    if (count < 2) return;

    std::vector<ContourPoint> expanded;
    expanded.reserve(count * 2);
    for (std::size_t i = 0; i < count; ++i) {
        const ContourPoint& current = points[i];
        const ContourPoint& next = points[(i + 1) % count];
        expanded.push_back(current);
        if (!current.onCurve && !next.onCurve) {
            expanded.push_back(ContourPoint{(current.position + next.position) * 0.5f, true});
        }
    }

    const std::size_t size = expanded.size();
    std::size_t start = 0;
    while (start < size && !expanded[start].onCurve) ++start;
    if (start == size) return;

    std::size_t index = start;
    glm::vec2 pen = expanded[start].position;
    for (std::size_t guard = 0; guard <= size; ++guard) {
        const std::size_t next = (index + 1) % size;
        if (expanded[next].onCurve) {
            if (expanded[next].position != pen) segments.push_back(OutlineSegment{pen, pen, expanded[next].position, false});
            pen = expanded[next].position;
            index = next;
        } else {
            const std::size_t end = (next + 1) % size;
            segments.push_back(OutlineSegment{pen, expanded[next].position, expanded[end].position, true});
            pen = expanded[end].position;
            index = end;
        }
        if (index == start) break;
    }
}

}

TrueType::TrueType(std::span<const std::uint8_t> bytes) : bytes_(bytes) {
    readMetrics();
    readCmap();
    readFamily();
    readKerning();
}

std::uint8_t TrueType::u8(std::size_t at) const {
    if (at >= bytes_.size()) malformed();
    return bytes_[at];
}

std::uint16_t TrueType::u16(std::size_t at) const {
    if (at + 2 > bytes_.size()) malformed();
    return static_cast<std::uint16_t>((bytes_[at] << 8) | bytes_[at + 1]);
}

std::int16_t TrueType::i16(std::size_t at) const { return static_cast<std::int16_t>(u16(at)); }

std::uint32_t TrueType::u32(std::size_t at) const {
    return (static_cast<std::uint32_t>(u16(at)) << 16) | u16(at + 2);
}

TrueType::Table TrueType::table(const char* tag) const {
    const std::uint16_t count = u16(4);
    for (std::uint16_t i = 0; i < count; ++i) {
        const std::size_t record = 12 + static_cast<std::size_t>(i) * 16;
        if (record + 16 > bytes_.size()) malformed();
        if (std::memcmp(bytes_.data() + record, tag, 4) != 0) continue;
        const Table found{u32(record + 8), u32(record + 12)};
        if (static_cast<std::size_t>(found.offset) + found.length > bytes_.size()) malformed();
        return found;
    }
    return {};
}

void TrueType::readMetrics() {
    const Table head = table("head");
    const Table maxp = table("maxp");
    const Table hhea = table("hhea");
    hmtx_ = table("hmtx");
    loca_ = table("loca");
    glyf_ = table("glyf");
    if (!head.present() || !maxp.present() || !hhea.present() || !hmtx_.present()) malformed();
    if (!loca_.present() || !glyf_.present()) throw std::runtime_error("only TrueType outlines are supported");

    unitsPerEm_ = u16(head.offset + 18);
    if (unitsPerEm_ == 0) malformed();
    longLoca_ = i16(head.offset + 50) != 0;
    glyphCount_ = u16(maxp.offset + 4);
    horizontalMetrics_ = u16(hhea.offset + 34);
    if (horizontalMetrics_ == 0 || static_cast<std::uint32_t>(horizontalMetrics_) * 4 > hmtx_.length) malformed();
    const std::uint32_t locaEntries = static_cast<std::uint32_t>(glyphCount_) + 1;
    if (locaEntries * (longLoca_ ? 4u : 2u) > loca_.length) malformed();

    ascender_ = i16(hhea.offset + 4);
    descender_ = i16(hhea.offset + 6);
    lineGap_ = i16(hhea.offset + 8);

    const Table os2 = table("OS/2");
    if (os2.length < 78) return;
    const int typoAscender = i16(os2.offset + 68);
    const int typoDescender = i16(os2.offset + 70);
    const int typoLineGap = i16(os2.offset + 72);
    const bool useTypo = (u16(os2.offset + 62) & 0x80u) != 0;
    if (useTypo || (ascender_ == 0 && descender_ == 0)) {
        if (useTypo || typoAscender != 0 || typoDescender != 0) {
            ascender_ = typoAscender;
            descender_ = typoDescender;
            lineGap_ = typoLineGap;
        } else {
            ascender_ = u16(os2.offset + 74);
            descender_ = -static_cast<int>(u16(os2.offset + 76));
            lineGap_ = 0;
        }
    }
}

void TrueType::readCmap() {
    const Table cmap = table("cmap");
    if (!cmap.present()) malformed();
    const std::uint16_t count = u16(cmap.offset + 2);
    std::size_t format4 = 0;
    std::size_t format12 = 0;
    for (std::uint16_t i = 0; i < count; ++i) {
        const std::size_t record = cmap.offset + 4 + static_cast<std::size_t>(i) * 8;
        const std::uint16_t platform = u16(record);
        const std::uint16_t encoding = u16(record + 2);
        const std::size_t subtable = cmap.offset + u32(record + 4);
        const std::uint16_t format = u16(subtable);
        const bool unicode = platform == 0 || (platform == 3 && (encoding == 1 || encoding == 10));
        if (!unicode) continue;
        if (format == 12 && format12 == 0) format12 = subtable;
        if (format == 4 && format4 == 0) format4 = subtable;
    }

    if (format12 != 0) {
        const std::uint32_t groups = u32(format12 + 12);
        if (format12 + 16 + static_cast<std::size_t>(groups) * 12 > bytes_.size()) malformed();
        cmap_ = format12;
        cmapFormat_ = 12;
    } else if (format4 != 0) {
        if (format4 + u16(format4 + 2) > bytes_.size()) malformed();
        cmap_ = format4;
        cmapFormat_ = 4;
    } else {
        throw std::runtime_error("the font has no Unicode character map");
    }
}

void TrueType::readFamily() {
    const Table name = table("name");
    if (!name.present()) return;
    const std::uint16_t count = u16(name.offset + 2);
    const std::size_t strings = name.offset + u16(name.offset + 4);

    int best = -1;
    int bestRank = 0;
    for (std::uint16_t i = 0; i < count; ++i) {
        const std::size_t record = name.offset + 6 + static_cast<std::size_t>(i) * 12;
        if (u16(record + 6) != 1) continue;
        const std::uint16_t platform = u16(record);
        const std::uint16_t language = u16(record + 4);
        int rank = 0;
        if (platform == 3) rank = language == 0x0409 ? 4 : 3;
        else if (platform == 0) rank = 2;
        else if (platform == 1) rank = 1;
        if (rank > bestRank) {
            bestRank = rank;
            best = i;
        }
    }
    if (best < 0) return;

    const std::size_t record = name.offset + 6 + static_cast<std::size_t>(best) * 12;
    const std::uint16_t length = u16(record + 8);
    const std::size_t at = strings + u16(record + 10);
    if (at + length > bytes_.size()) malformed();

    if (bestRank == 1) {
        for (std::uint16_t i = 0; i < length; ++i) {
            const std::uint8_t byte = bytes_[at + i];
            family_.push_back(byte < 0x80 ? static_cast<char>(byte) : '?');
        }
        return;
    }
    for (std::size_t i = 0; i + 1 < length; i += 2) {
        char32_t unit = u16(at + i);
        if (unit >= 0xD800 && unit < 0xDC00 && i + 3 < length) {
            const char32_t low = u16(at + i + 2);
            if (low >= 0xDC00 && low < 0xE000) {
                unit = 0x10000 + ((unit - 0xD800) << 10) + (low - 0xDC00);
                i += 2;
            }
        }
        cinder::Platform::appendUtf8(family_, unit);
    }
}

void TrueType::readKerning() {
    const Table gpos = table("GPOS");
    if (gpos.length < 10) return;
    const std::size_t scripts = gpos.offset + u16(gpos.offset + 4);
    const std::size_t features = gpos.offset + u16(gpos.offset + 6);
    const std::size_t lookups = gpos.offset + u16(gpos.offset + 8);

    const std::uint16_t scriptCount = u16(scripts);
    if (scriptCount == 0) return;
    std::size_t script = 0;
    for (const char* wanted : {"DFLT", "latn"}) {
        for (std::uint16_t i = 0; i < scriptCount && script == 0; ++i) {
            const std::size_t record = scripts + 2 + static_cast<std::size_t>(i) * 6;
            if (record + 6 > bytes_.size()) malformed();
            if (std::memcmp(bytes_.data() + record, wanted, 4) == 0) script = scripts + u16(record + 4);
        }
    }
    if (script == 0) script = scripts + u16(scripts + 2 + 4);
    const std::uint16_t langSysOffset = u16(script);
    if (langSysOffset == 0) return;
    const std::size_t langSys = script + langSysOffset;

    std::vector<std::uint16_t> featureIndices;
    if (const std::uint16_t required = u16(langSys + 2); required != NO_REQUIRED_FEATURE) {
        featureIndices.push_back(required);
    }
    const std::uint16_t featureIndexCount = u16(langSys + 4);
    for (std::uint16_t i = 0; i < featureIndexCount; ++i) featureIndices.push_back(u16(langSys + 6 + 2 * i));

    const std::uint16_t featureCount = u16(features);
    std::vector<std::uint16_t> lookupIndices;
    for (const std::uint16_t index : featureIndices) {
        if (index >= featureCount) continue;
        const std::size_t record = features + 2 + static_cast<std::size_t>(index) * 6;
        if (record + 6 > bytes_.size()) malformed();
        if (std::memcmp(bytes_.data() + record, "kern", 4) != 0) continue;
        const std::size_t feature = features + u16(record + 4);
        const std::uint16_t count = u16(feature + 2);
        for (std::uint16_t i = 0; i < count; ++i) lookupIndices.push_back(u16(feature + 4 + 2 * i));
    }
    std::sort(lookupIndices.begin(), lookupIndices.end());
    lookupIndices.erase(std::unique(lookupIndices.begin(), lookupIndices.end()), lookupIndices.end());

    const std::uint16_t lookupCount = u16(lookups);
    for (const std::uint16_t index : lookupIndices) {
        if (index >= lookupCount) continue;
        const std::size_t lookup = lookups + u16(lookups + 2 + 2 * index);
        const std::uint16_t type = u16(lookup);
        const std::uint16_t subtableCount = u16(lookup + 4);
        std::vector<std::size_t> subtables;
        for (std::uint16_t i = 0; i < subtableCount; ++i) {
            std::size_t subtable = lookup + u16(lookup + 6 + 2 * i);
            if (type == 9) {
                if (u16(subtable) != 1 || u16(subtable + 2) != 2) continue;
                subtable += u32(subtable + 4);
            } else if (type != 2) {
                continue;
            }
            subtables.push_back(subtable);
        }
        if (!subtables.empty()) kernLookups_.push_back(std::move(subtables));
    }
}

std::uint32_t TrueType::glyphIndex(char32_t codepoint) const {
    try {
        return cmapFormat_ == 12 ? lookupFormat12(codepoint) : lookupFormat4(codepoint);
    } catch (const std::runtime_error&) {
        return 0;
    }
}

std::uint32_t TrueType::lookupFormat4(char32_t codepoint) const {
    if (codepoint > 0xFFFF) return 0;
    const std::size_t segmentsX2 = u16(cmap_ + 6);
    const std::size_t segments = segmentsX2 / 2;
    const std::size_t ends = cmap_ + 14;
    const std::size_t starts = ends + segmentsX2 + 2;
    const std::size_t deltas = starts + segmentsX2;
    const std::size_t ranges = deltas + segmentsX2;

    std::size_t low = 0;
    std::size_t high = segments;
    while (low < high) {
        const std::size_t middle = (low + high) / 2;
        if (u16(ends + middle * 2) < codepoint) low = middle + 1;
        else high = middle;
    }
    if (low == segments) return 0;
    const std::uint16_t start = u16(starts + low * 2);
    if (codepoint < start) return 0;

    const std::uint16_t delta = u16(deltas + low * 2);
    const std::size_t rangeAt = ranges + low * 2;
    const std::uint16_t rangeOffset = u16(rangeAt);
    if (rangeOffset == 0) return (codepoint + delta) & 0xFFFFu;
    const std::uint16_t glyph = u16(rangeAt + rangeOffset + (codepoint - start) * 2);
    return glyph == 0 ? 0 : (glyph + delta) & 0xFFFFu;
}

std::uint32_t TrueType::lookupFormat12(char32_t codepoint) const {
    const std::uint32_t groups = u32(cmap_ + 12);
    std::uint32_t low = 0;
    std::uint32_t high = groups;
    while (low < high) {
        const std::uint32_t middle = (low + high) / 2;
        const std::size_t group = cmap_ + 16 + static_cast<std::size_t>(middle) * 12;
        if (u32(group + 4) < codepoint) low = middle + 1;
        else high = middle;
    }
    if (low == groups) return 0;
    const std::size_t group = cmap_ + 16 + static_cast<std::size_t>(low) * 12;
    const std::uint32_t start = u32(group);
    if (codepoint < start) return 0;
    const std::uint32_t glyph = u32(group + 8) + (codepoint - start);
    return glyph < static_cast<std::uint32_t>(glyphCount_) ? glyph : 0;
}

int TrueType::advance(std::uint32_t glyph) const {
    const std::uint32_t metric = std::min<std::uint32_t>(glyph, static_cast<std::uint32_t>(horizontalMetrics_ - 1));
    return u16(hmtx_.offset + static_cast<std::size_t>(metric) * 4);
}

int TrueType::coverageIndex(std::size_t coverage, std::uint32_t glyph) const {
    const std::uint16_t format = u16(coverage);
    const std::uint16_t count = u16(coverage + 2);
    std::uint16_t low = 0;
    std::uint16_t high = count;
    if (format == 1) {
        while (low < high) {
            const std::uint16_t middle = static_cast<std::uint16_t>((low + high) / 2);
            const std::uint16_t value = u16(coverage + 4 + 2 * static_cast<std::size_t>(middle));
            if (value == glyph) return middle;
            if (value < glyph) low = static_cast<std::uint16_t>(middle + 1);
            else high = middle;
        }
        return -1;
    }
    if (format != 2) return -1;
    while (low < high) {
        const std::uint16_t middle = static_cast<std::uint16_t>((low + high) / 2);
        const std::size_t range = coverage + 4 + 6 * static_cast<std::size_t>(middle);
        if (u16(range + 2) < glyph) low = static_cast<std::uint16_t>(middle + 1);
        else high = middle;
    }
    if (low == count) return -1;
    const std::size_t range = coverage + 4 + 6 * static_cast<std::size_t>(low);
    const std::uint16_t start = u16(range);
    if (glyph < start) return -1;
    return static_cast<int>(u16(range + 4) + (glyph - start));
}

int TrueType::glyphClass(std::size_t classDef, std::uint32_t glyph) const {
    const std::uint16_t format = u16(classDef);
    if (format == 1) {
        const std::uint16_t start = u16(classDef + 2);
        const std::uint16_t count = u16(classDef + 4);
        if (glyph < start || glyph >= static_cast<std::uint32_t>(start) + count) return 0;
        return u16(classDef + 6 + 2 * static_cast<std::size_t>(glyph - start));
    }
    if (format != 2) return 0;
    const std::uint16_t count = u16(classDef + 2);
    std::uint16_t low = 0;
    std::uint16_t high = count;
    while (low < high) {
        const std::uint16_t middle = static_cast<std::uint16_t>((low + high) / 2);
        const std::size_t range = classDef + 4 + 6 * static_cast<std::size_t>(middle);
        if (u16(range + 2) < glyph) low = static_cast<std::uint16_t>(middle + 1);
        else high = middle;
    }
    if (low == count) return 0;
    const std::size_t range = classDef + 4 + 6 * static_cast<std::size_t>(low);
    return glyph < u16(range) ? 0 : u16(range + 4);
}

bool TrueType::pairAdjustment(std::size_t subtable, std::uint32_t left, std::uint32_t right, int& adjustment) const {
    const std::uint16_t format = u16(subtable);
    const int index = coverageIndex(subtable + u16(subtable + 2), left);
    if (index < 0) return false;

    const std::uint16_t format1 = u16(subtable + 4);
    const std::uint16_t format2 = u16(subtable + 6);
    const std::size_t size1 = static_cast<std::size_t>(valueSize(format1));
    const std::size_t size2 = static_cast<std::size_t>(valueSize(format2));
    const auto xAdvance = [&](std::size_t value) {
        if ((format1 & VALUE_X_ADVANCE) == 0) return 0;
        return static_cast<int>(i16(value + static_cast<std::size_t>(valueSize(format1 & 0x3u))));
    };

    if (format == 1) {
        if (index >= u16(subtable + 8)) return false;
        const std::size_t pairSet = subtable + u16(subtable + 10 + 2 * static_cast<std::size_t>(index));
        const std::size_t recordSize = 2 + size1 + size2;
        std::uint16_t low = 0;
        std::uint16_t high = u16(pairSet);
        while (low < high) {
            const std::uint16_t middle = static_cast<std::uint16_t>((low + high) / 2);
            const std::size_t record = pairSet + 2 + recordSize * middle;
            const std::uint16_t second = u16(record);
            if (second == right) {
                adjustment = xAdvance(record + 2);
                return true;
            }
            if (second < right) low = static_cast<std::uint16_t>(middle + 1);
            else high = middle;
        }
        return false;
    }
    if (format != 2) return false;

    const std::size_t classDef1 = subtable + u16(subtable + 8);
    const std::size_t classDef2 = subtable + u16(subtable + 10);
    const std::uint16_t class1Count = u16(subtable + 12);
    const std::uint16_t class2Count = u16(subtable + 14);
    const int class1 = glyphClass(classDef1, left);
    const int class2 = glyphClass(classDef2, right);
    if (class1 >= class1Count || class2 >= class2Count) return false;
    const std::size_t record =
        subtable + 16 + (static_cast<std::size_t>(class1) * class2Count + static_cast<std::size_t>(class2)) * (size1 + size2);
    adjustment = xAdvance(record);
    return true;
}

int TrueType::kerning(std::uint32_t left, std::uint32_t right) const {
    try {
        int total = 0;
        for (const std::vector<std::size_t>& lookup : kernLookups_) {
            for (const std::size_t subtable : lookup) {
                int adjustment = 0;
                if (pairAdjustment(subtable, left, right, adjustment)) {
                    total += adjustment;
                    break;
                }
            }
        }
        return total;
    } catch (const std::runtime_error&) {
        return 0;
    }
}

Outline TrueType::outline(std::uint32_t glyph) const {
    Outline result;
    try {
        appendGlyph(glyph, glm::mat2(1.0f), glm::vec2(0.0f), 0, result.segments);
    } catch (const std::runtime_error&) {
        return {};
    }
    if (result.segments.empty()) return result;
    result.min = result.segments.front().from;
    result.max = result.min;
    for (const OutlineSegment& segment : result.segments) {
        for (const glm::vec2& point : {segment.from, segment.control, segment.to}) {
            result.min = glm::min(result.min, point);
            result.max = glm::max(result.max, point);
        }
    }
    return result;
}

void TrueType::appendGlyph(std::uint32_t glyph, const glm::mat2& transform, glm::vec2 offset, int depth,
                           std::vector<OutlineSegment>& segments) const {
    if (depth > MAX_COMPOSITE_DEPTH || glyph >= static_cast<std::uint32_t>(glyphCount_)) return;
    const std::size_t entry = static_cast<std::size_t>(glyph);
    const std::uint32_t start = longLoca_ ? u32(loca_.offset + entry * 4) : 2u * u16(loca_.offset + entry * 2);
    const std::uint32_t end = longLoca_ ? u32(loca_.offset + entry * 4 + 4) : 2u * u16(loca_.offset + entry * 2 + 2);
    if (end <= start) return;
    if (end > glyf_.length) malformed();
    const std::size_t at = glyf_.offset + start;
    const std::int16_t contours = i16(at);

    if (contours >= 0) {
        if (contours == 0) return;
        const std::size_t endPoints = at + 10;
        const std::size_t pointCount = static_cast<std::size_t>(u16(endPoints + 2 * (contours - 1))) + 1;
        const std::size_t instructions = endPoints + 2 * static_cast<std::size_t>(contours);
        std::size_t cursor = instructions + 2 + u16(instructions);

        std::vector<std::uint8_t> flags;
        flags.reserve(pointCount);
        while (flags.size() < pointCount) {
            const std::uint8_t flag = u8(cursor++);
            flags.push_back(flag);
            if ((flag & REPEAT) != 0) {
                const std::uint8_t repeats = u8(cursor++);
                for (int i = 0; i < repeats && flags.size() < pointCount; ++i) flags.push_back(flag);
            }
        }

        std::vector<glm::vec2> points(pointCount);
        int x = 0;
        for (std::size_t i = 0; i < pointCount; ++i) {
            if ((flags[i] & X_SHORT) != 0) {
                const int delta = u8(cursor++);
                x += (flags[i] & X_SAME_OR_POSITIVE) != 0 ? delta : -delta;
            } else if ((flags[i] & X_SAME_OR_POSITIVE) == 0) {
                x += i16(cursor);
                cursor += 2;
            }
            points[i].x = static_cast<float>(x);
        }
        int y = 0;
        for (std::size_t i = 0; i < pointCount; ++i) {
            if ((flags[i] & Y_SHORT) != 0) {
                const int delta = u8(cursor++);
                y += (flags[i] & Y_SAME_OR_POSITIVE) != 0 ? delta : -delta;
            } else if ((flags[i] & Y_SAME_OR_POSITIVE) == 0) {
                y += i16(cursor);
                cursor += 2;
            }
            points[i].y = static_cast<float>(y);
        }

        std::size_t first = 0;
        std::vector<ContourPoint> contour;
        for (std::int16_t c = 0; c < contours; ++c) {
            const std::size_t last = u16(endPoints + 2 * static_cast<std::size_t>(c));
            if (last < first || last >= pointCount) malformed();
            contour.clear();
            for (std::size_t i = first; i <= last; ++i) {
                contour.push_back(ContourPoint{transform * points[i] + offset, (flags[i] & ON_CURVE) != 0});
            }
            appendContour(contour, segments);
            first = last + 1;
        }
        return;
    }

    std::size_t cursor = at + 10;
    for (;;) {
        const std::uint16_t flags = u16(cursor);
        const std::uint16_t component = u16(cursor + 2);
        cursor += 4;
        int argument1 = 0;
        int argument2 = 0;
        if ((flags & ARG_1_AND_2_ARE_WORDS) != 0) {
            argument1 = i16(cursor);
            argument2 = i16(cursor + 2);
            cursor += 4;
        } else {
            argument1 = static_cast<std::int8_t>(u8(cursor));
            argument2 = static_cast<std::int8_t>(u8(cursor + 1));
            cursor += 2;
        }

        glm::mat2 local(1.0f);
        if ((flags & WE_HAVE_A_SCALE) != 0) {
            const float scale = f2dot14(i16(cursor));
            local = glm::mat2(scale, 0.0f, 0.0f, scale);
            cursor += 2;
        } else if ((flags & X_AND_Y_SCALE) != 0) {
            local = glm::mat2(f2dot14(i16(cursor)), 0.0f, 0.0f, f2dot14(i16(cursor + 2)));
            cursor += 4;
        } else if ((flags & TWO_BY_TWO) != 0) {
            local = glm::mat2(f2dot14(i16(cursor)), f2dot14(i16(cursor + 2)), f2dot14(i16(cursor + 4)),
                              f2dot14(i16(cursor + 6)));
            cursor += 8;
        }

        const glm::vec2 shift = (flags & ARGS_ARE_XY_VALUES) != 0
                                    ? glm::vec2(static_cast<float>(argument1), static_cast<float>(argument2))
                                    : glm::vec2(0.0f);
        appendGlyph(component, transform * local, transform * shift + offset, depth + 1, segments);
        if ((flags & MORE_COMPONENTS) == 0) break;
    }
}

}
