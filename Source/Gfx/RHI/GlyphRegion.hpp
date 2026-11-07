#pragma once

#include <cstddef>
#include <cstdint>

namespace cinder::Gfx::RHI {

struct GlyphRegion {
    std::size_t page = 0;
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::int32_t width = 0;
    std::int32_t height = 0;
    std::uint64_t offset = 0;
};

}
