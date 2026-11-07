#include "gfx/rhi/Capture.hpp"

#include "platform/Log.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <algorithm>
#include <cstring>
#include <vector>

namespace cinder::gfx::rhi {
namespace {

constexpr int BYTES_PER_PIXEL = 4;

}

bool writeCapture(const std::string& path, uint32_t width, uint32_t height, const void* data, Format format,
                  std::size_t rowBytes) {
    const std::size_t packed = static_cast<std::size_t>(width) * BYTES_PER_PIXEL;
    const std::size_t stride = rowBytes == 0 ? packed : rowBytes;
    std::vector<unsigned char> pixels(packed * height);
    const auto* source = static_cast<const unsigned char*>(data);
    for (std::uint32_t row = 0; row < height; ++row) {
        std::memcpy(pixels.data() + row * packed, source + row * stride, packed);
    }

    if (isBgra(format)) {
        for (std::size_t i = 0; i < pixels.size(); i += 4) std::swap(pixels[i], pixels[i + 2]);
    }

    const int written = stbi_write_png(path.c_str(), static_cast<int>(width), static_cast<int>(height),
                                       BYTES_PER_PIXEL, pixels.data(), static_cast<int>(packed));
    if (written == 0) {
        cinder::platform::logError("[capture] could not write %s\n", path.c_str());
        return false;
    }
    cinder::platform::logInfo("[capture] wrote %s\n", path.c_str());
    return true;
}

}
