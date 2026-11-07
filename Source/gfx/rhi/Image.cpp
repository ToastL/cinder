#include "gfx/rhi/Image.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <memory>
#include <stdexcept>

namespace cinder::gfx::rhi {

Image loadImage(const std::string& path) {
    int width = 0;
    int height = 0;
    int channels = 0;
    const std::unique_ptr<stbi_uc, void (*)(void*)> pixels(
            stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha), stbi_image_free);
    if (!pixels) throw std::runtime_error("Failed to load " + path + ": " + stbi_failure_reason());
    const std::size_t bytes = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4;
    return Image{static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height),
                 std::vector<unsigned char>(pixels.get(), pixels.get() + bytes)};
}

}
