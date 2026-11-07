#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace cinder::gfx::rhi {

struct Image {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::vector<unsigned char> pixels;
};

Image loadImage(const std::string& path);

}
