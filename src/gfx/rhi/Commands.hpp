#pragma once

#include "gfx/rhi/Handles.hpp"

#include <cstdint>

namespace cinder::gfx::rhi {

void draw(Commands cmd, std::uint32_t vertexCount);
void drawIndexed(Commands cmd, std::uint32_t indexCount, std::uint32_t firstIndex);
void viewport(Commands cmd, std::uint32_t width, std::uint32_t height);
void scissor(Commands cmd, std::int32_t x, std::int32_t y, std::uint32_t width,
             std::uint32_t height);

}
