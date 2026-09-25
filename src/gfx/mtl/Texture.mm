#include "gfx/mtl/Texture.hpp"

#include "gfx/mtl/Ctx.hpp"
#include "gfx/mtl/MetalUtil.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <stdexcept>
#include <utility>

namespace cinder::gfx::rhi {

Texture::Texture(const Ctx& ctx, const unsigned char* pixels, std::uint32_t width,
                 std::uint32_t height)
    : width_(width), height_(height) {
    id<MTLDevice> device = cinder::gfx::mtl::bridge<id<MTLDevice>>(ctx.device());
    MTLTextureDescriptor* desc = [MTLTextureDescriptor
            texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm_sRGB
                                         width:width height:height mipmapped:NO];
    desc.storageMode = MTLStorageModeShared;
    desc.usage = MTLTextureUsageShaderRead;
    id<MTLTexture> texture = [device newTextureWithDescriptor:desc];
    if (texture == nil) throw cinder::gfx::mtl::error("Could not create Metal texture");
    [texture replaceRegion:MTLRegionMake2D(0, 0, width, height) mipmapLevel:0
                 withBytes:pixels bytesPerRow:static_cast<NSUInteger>(width) * 4];
    handle_ = cinder::gfx::mtl::retain(texture);
}

Texture Texture::load(const Ctx& ctx, const std::string& path) {
    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_uc* pixels = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (pixels == nullptr) {
        throw std::runtime_error("Failed to load " + path + ": " + stbi_failure_reason());
    }
    Texture texture(ctx, pixels, static_cast<std::uint32_t>(width),
                    static_cast<std::uint32_t>(height));
    stbi_image_free(pixels);
    return texture;
}

Texture Texture::white(const Ctx& ctx) {
    const unsigned char pixels[] = {0xff, 0xff, 0xff, 0xff};
    return Texture(ctx, pixels, 1, 1);
}

Texture::Texture(Texture&& other) noexcept
    : handle_(std::exchange(other.handle_, nullptr)), binding_(other.binding_),
      width_(other.width_), height_(other.height_) {}

Texture::~Texture() { cinder::gfx::mtl::release(handle_); }

}
