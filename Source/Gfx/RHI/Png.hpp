#pragma once

#include "Gfx/RHI/Image.hpp"

#include <cstdint>
#include <string>

namespace cinder::Gfx::RHI {

Image readPng(const std::string& path);

bool writePng(const std::string& path, std::uint32_t width, std::uint32_t height, const unsigned char* rgba);

}
