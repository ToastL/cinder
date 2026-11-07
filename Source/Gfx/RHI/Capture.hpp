#pragma once

#include "Gfx/RHI/Format.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

namespace cinder::Gfx::RHI {

bool writeCapture(const std::string& path, std::uint32_t width, std::uint32_t height, const void* pixels,
                  Format format, std::size_t rowBytes = 0);

}
