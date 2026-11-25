#include "Gfx/MTL/VolumeTexture.hpp"

#include "Gfx/MTL/Ctx.hpp"
#include "Gfx/MTL/Formats.hpp"
#include "Gfx/MTL/MetalUtil.hpp"

#include <stdexcept>
#include <utility>

namespace cinder::Gfx::RHI {

namespace {

std::uint32_t bytesPerVoxel(Format format) {
    switch (format) {
        case Format::R8Unorm: return 1;
        case Format::RGBA8Unorm:
        case Format::RGBA8Srgb:
        case Format::BGRA8Unorm:
        case Format::BGRA8Srgb: return 4;
        case Format::RGBA16Sfloat: return 8;
        default: return 0;
    }
}

}

VolumeTexture::VolumeTexture(const Ctx& ctx, const void* data, std::uint32_t width,
                             std::uint32_t height, std::uint32_t depth, Format format)
    : width_(width), height_(height), depth_(depth) {
    const std::uint32_t voxelBytes = bytesPerVoxel(format);
    if (voxelBytes == 0) throw std::runtime_error("VolumeTexture: unsupported format");

    const cinder::Gfx::MTL::PoolScope pool;
    id<MTLDevice> device = cinder::Gfx::MTL::bridge<id<MTLDevice>>(ctx.device());

    MTLTextureDescriptor* desc = [[MTLTextureDescriptor alloc] init];
    desc.textureType = MTLTextureType3D;
    desc.pixelFormat = static_cast<MTLPixelFormat>(cinder::Gfx::MTL::pixelFormat(format));
    desc.width = width;
    desc.height = height;
    desc.depth = depth;
    desc.mipmapLevelCount = 1;
    desc.storageMode = MTLStorageModeShared;
    desc.usage = MTLTextureUsageShaderRead;

    id<MTLTexture> texture = [device newTextureWithDescriptor:desc];
    if (texture == nil) throw cinder::Gfx::MTL::error("Could not create Metal 3D texture");

    const NSUInteger rowBytes = static_cast<NSUInteger>(width) * voxelBytes;
    const NSUInteger sliceBytes = rowBytes * height;
    [texture replaceRegion:MTLRegionMake3D(0, 0, 0, width, height, depth) mipmapLevel:0 slice:0
                 withBytes:data bytesPerRow:rowBytes bytesPerImage:sliceBytes];

    handle_ = cinder::Gfx::MTL::retain(texture);
}

VolumeTexture::VolumeTexture(VolumeTexture&& other) noexcept
    : handle_(std::exchange(other.handle_, nullptr)), binding_(other.binding_),
      width_(other.width_), height_(other.height_), depth_(other.depth_) {}

VolumeTexture::~VolumeTexture() { cinder::Gfx::MTL::release(handle_); }

}
