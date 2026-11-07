#pragma once

#include "Gfx/RHI/Format.hpp"

#include <cstdint>

namespace cinder::Gfx::MTL {

std::uint64_t pixelFormat(cinder::Gfx::RHI::Format format);
std::uint64_t vertexFormat(cinder::Gfx::RHI::VertexFormat format);

}
