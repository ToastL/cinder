#pragma once

#include "gfx/rhi/Format.hpp"

#include <cstdint>

namespace cinder::gfx::mtl {

std::uint64_t pixelFormat(cinder::gfx::rhi::Format format);
std::uint64_t vertexFormat(cinder::gfx::rhi::VertexFormat format);

}
