#include "Gfx/MTL/Texture.hpp"

#include "Gfx/MTL/Ctx.hpp"
#include "Gfx/MTL/MetalUtil.hpp"
#include "Gfx/RHI/Image.hpp"

#include <stdexcept>
#include <utility>

namespace cinder::Gfx::RHI {

Texture::Texture(const Ctx& ctx, const unsigned char* pixels, std::uint32_t width,
                 std::uint32_t height)
    : width_(width), height_(height) {
    const cinder::Gfx::MTL::PoolScope pool;
    id<MTLDevice> device = cinder::Gfx::MTL::bridge<id<MTLDevice>>(ctx.device());
    MTLTextureDescriptor* desc = [MTLTextureDescriptor
            texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm_sRGB
                                         width:width height:height mipmapped:NO];
    desc.storageMode = MTLStorageModeShared;
    desc.usage = MTLTextureUsageShaderRead;
    id<MTLTexture> texture = [device newTextureWithDescriptor:desc];
    if (texture == nil) throw cinder::Gfx::MTL::error("Could not create Metal texture");
    [texture replaceRegion:MTLRegionMake2D(0, 0, width, height) mipmapLevel:0
                 withBytes:pixels bytesPerRow:static_cast<NSUInteger>(width) * 4];
    handle_ = cinder::Gfx::MTL::retain(texture);
}

Texture Texture::load(const Ctx& ctx, const std::string& path) {
    const Image image = loadImage(path);
    return Texture(ctx, image.pixels.data(), image.width, image.height);
}

Texture Texture::white(const Ctx& ctx) {
    const unsigned char pixels[] = {0xff, 0xff, 0xff, 0xff};
    return Texture(ctx, pixels, 1, 1);
}

Texture::Texture(Texture&& other) noexcept
    : handle_(std::exchange(other.handle_, nullptr)), binding_(other.binding_),
      width_(other.width_), height_(other.height_) {}

Texture::~Texture() { cinder::Gfx::MTL::release(handle_); }

}
