#include "gfx/rhi/Capture.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <algorithm>
#include <cstring>
#include <vector>

namespace cinder::gfx::rhi {

void writeCapture(const std::string& path, uint32_t width, uint32_t height, const void* data,
                  Format format) {
    const std::size_t size = static_cast<std::size_t>(width) * height * 4;
    std::vector<unsigned char> pixels(size);
    std::memcpy(pixels.data(), data, size);

    if (isBgra(format)) {
        for (std::size_t i = 0; i < pixels.size(); i += 4) std::swap(pixels[i], pixels[i + 2]);
    }

    stbi_write_png(path.c_str(), static_cast<int>(width), static_cast<int>(height), 4,
                   pixels.data(), static_cast<int>(width) * 4);
}

}
