#pragma once

#include <cstddef>
#include <string_view>
#include <vector>

namespace cinder::text {

struct ShapedText;

struct CaretStops {
    std::vector<std::size_t> offsets;
    std::vector<float> positions;

    float positionOf(std::size_t offset) const;
    std::size_t nearest(float x) const;
};

CaretStops caretStops(const ShapedText& shaped, std::string_view utf8);

}
