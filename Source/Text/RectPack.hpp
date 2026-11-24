#pragma once

#include <optional>
#include <vector>

namespace cinder::Text {

struct PackedRect {
    int x;
    int y;
};

class RectPacker {
public:
    RectPacker(int width, int height);

    std::optional<PackedRect> pack(int width, int height);
    void reset();

private:
    struct Segment {
        int x;
        int y;
        int width;
    };

    int width_;
    int height_;
    std::vector<Segment> skyline_;
};

}
