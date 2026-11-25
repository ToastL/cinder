#pragma once

#include <cstdint>
#include <vector>

struct CloudNoise {
    static constexpr std::uint32_t SHAPE_SIZE = 128;
    static constexpr std::uint32_t DETAIL_SIZE = 32;
    static constexpr std::uint32_t WEATHER_SIZE = 256;

    std::vector<std::uint8_t> shape;
    std::vector<std::uint8_t> detail;
    std::vector<std::uint8_t> weather;
};

const CloudNoise& cloudNoise();
