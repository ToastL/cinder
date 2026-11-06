#pragma once

#include "gfx/rhi/Format.hpp"

#include <cstdint>
#include <string>

namespace cinder::gfx::rhi {

void writeCapture(const std::string& path, std::uint32_t width, std::uint32_t height,
                  const void* pixels, Format format);

}
