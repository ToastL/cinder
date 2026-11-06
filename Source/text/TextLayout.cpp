#include "text/TextLayout.hpp"

#include "text/Shaper.hpp"
#include "text/Utf8.hpp"

#include <algorithm>
#include <cmath>
#include <map>

namespace cinder::text {

float CaretStops::positionOf(std::size_t offset) const {
    const auto found = std::lower_bound(offsets.begin(), offsets.end(), offset);
    if (found == offsets.end()) return positions.empty() ? 0.0f : positions.back();
    return positions[static_cast<std::size_t>(found - offsets.begin())];
}

std::size_t CaretStops::nearest(float x) const {
    if (offsets.empty()) return 0;
    std::size_t best = 0;
    float distance = std::abs(positions[0] - x);
    for (std::size_t i = 1; i < positions.size(); ++i) {
        const float candidate = std::abs(positions[i] - x);
        if (candidate < distance) {
            distance = candidate;
            best = i;
        }
    }
    return offsets[best];
}

CaretStops caretStops(const ShapedText& shaped, std::string_view utf8) {
    struct Cluster {
        float start = 0.0f;
        float width = 0.0f;
    };
    std::map<std::size_t, Cluster> clusters;
    float pen = 0.0f;
    for (const ShapedGlyph& glyph : shaped.glyphs) {
        auto [entry, inserted] = clusters.try_emplace(glyph.cluster, Cluster{pen, 0.0f});
        entry->second.width += glyph.advance;
        pen += glyph.advance;
    }

    CaretStops stops;
    std::size_t offset = 0;
    while (true) {
        float position = pen;
        if (offset < utf8.size() && !clusters.empty()) {
            auto cluster = clusters.upper_bound(offset);
            if (cluster != clusters.begin()) {
                --cluster;
                const std::size_t start = cluster->first;
                const auto next = std::next(cluster);
                const std::size_t end = next == clusters.end() ? utf8.size() : next->first;
                const auto total = static_cast<float>(characterCount(utf8.substr(start, end - start)));
                const auto before = static_cast<float>(characterCount(utf8.substr(start, offset - start)));
                position = cluster->second.start + (total > 0.0f ? cluster->second.width * before / total : 0.0f);
            } else {
                position = 0.0f;
            }
        } else if (utf8.empty()) {
            position = 0.0f;
        }
        stops.offsets.push_back(offset);
        stops.positions.push_back(position);
        if (offset >= utf8.size()) break;
        offset = nextBoundary(utf8, offset);
    }
    return stops;
}

}
