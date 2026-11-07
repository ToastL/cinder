#pragma once

#include "gfx/rhi/Format.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

namespace cinder::gfx::rhi {

bool writeCapture(const std::string& path, std::uint32_t width, std::uint32_t height, const void* pixels,
                  Format format, std::size_t rowBytes = 0);

}
