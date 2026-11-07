#pragma once

#include "Gfx/RHI/Handles.hpp"

#include <cstdint>

namespace cinder::Gfx::RHI {

void draw(Commands cmd, std::uint32_t vertexCount);
void drawIndexed(Commands cmd, std::uint32_t indexCount, std::uint32_t firstIndex);
void viewport(Commands cmd, std::uint32_t width, std::uint32_t height);
void scissor(Commands cmd, std::int32_t x, std::int32_t y, std::uint32_t width,
             std::uint32_t height);

}
