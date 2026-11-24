#include "Gfx/RHI/Capture.hpp"

#include "Gfx/RHI/Png.hpp"
#include "Platform/Log.hpp"

#include <algorithm>
#include <cstring>
#include <vector>

namespace cinder::Gfx::RHI {
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

    if (!writePng(path, width, height, pixels.data())) {
        cinder::Platform::logError("[capture] could not write %s\n", path.c_str());
        return false;
    }
    cinder::Platform::logInfo("[capture] wrote %s\n", path.c_str());
    return true;
}

}
