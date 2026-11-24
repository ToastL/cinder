#include "Text/RectPack.hpp"

#include <algorithm>

namespace cinder::Text {

RectPacker::RectPacker(int width, int height) : width_(width), height_(height) { reset(); }

void RectPacker::reset() {
    skyline_.clear();
    skyline_.push_back({0, 0, width_});
}

std::optional<PackedRect> RectPacker::pack(int width, int height) {
    if (width <= 0 || height <= 0 || width > width_ || height > height_) return std::nullopt;

    int bestX = -1;
    int bestY = height_ + 1;
    int bestIndex = -1;

    for (std::size_t i = 0; i < skyline_.size(); ++i) {
        const int x = skyline_[i].x;
        if (x + width > width_) break;

        int y = 0;
        int covered = 0;
        for (std::size_t j = i; j < skyline_.size() && covered < width; ++j) {
            y = std::max(y, skyline_[j].y);
            covered += skyline_[j].width;
        }
        if (covered < width) continue;

        if (y + height <= height_ && y < bestY) {
            bestY = y;
            bestX = x;
            bestIndex = static_cast<int>(i);
        }
    }

    if (bestIndex < 0) return std::nullopt;

    const int rightEdge = bestX + width;
    const int newTop = bestY + height;
    std::vector<Segment> rebuilt;
    rebuilt.reserve(skyline_.size() + 1);

    for (int i = 0; i < bestIndex; ++i) rebuilt.push_back(skyline_[static_cast<std::size_t>(i)]);
    rebuilt.push_back({bestX, newTop, width});

    std::size_t i = static_cast<std::size_t>(bestIndex);
    while (i < skyline_.size()) {
        const int segRight = skyline_[i].x + skyline_[i].width;
        if (segRight <= rightEdge) {
            ++i;
        } else {
            const int newWidth = segRight - rightEdge;
            rebuilt.push_back({rightEdge, skyline_[i].y, newWidth});
            ++i;
            break;
        }
    }
    while (i < skyline_.size()) rebuilt.push_back(skyline_[i++]);

    skyline_.clear();
    skyline_.reserve(rebuilt.size());
    for (const Segment& seg : rebuilt) {
        if (!skyline_.empty() && skyline_.back().y == seg.y) {
            skyline_.back().width += seg.width;
        } else {
            skyline_.push_back(seg);
        }
    }

    return PackedRect{bestX, bestY};
}

}
