#include "gfx/vk/Formats.hpp"

#include "platform/Log.hpp"

namespace cinder::gfx::rhi {
namespace {

struct Pair {
    Format format;
    VkFormat vk;
};

constexpr Pair TABLE[] = {
    {Format::Undefined, VK_FORMAT_UNDEFINED},
    {Format::BGRA8Srgb, VK_FORMAT_B8G8R8A8_SRGB},
    {Format::BGRA8Unorm, VK_FORMAT_B8G8R8A8_UNORM},
    {Format::RGBA8Srgb, VK_FORMAT_R8G8B8A8_SRGB},
    {Format::ABGR8SrgbPack32, VK_FORMAT_A8B8G8R8_SRGB_PACK32},
    {Format::R8Unorm, VK_FORMAT_R8_UNORM},
    {Format::D32Sfloat, VK_FORMAT_D32_SFLOAT},
    {Format::D32SfloatS8Uint, VK_FORMAT_D32_SFLOAT_S8_UINT},
    {Format::D24UnormS8Uint, VK_FORMAT_D24_UNORM_S8_UINT},
};

}

VkFormat toVk(Format format) {
    for (const Pair& pair : TABLE) {
        if (pair.format == format) return pair.vk;
    }
    return VK_FORMAT_UNDEFINED;
}

Format fromVk(VkFormat format) {
    for (const Pair& pair : TABLE) {
        if (pair.vk == format) return pair.format;
    }
    cinder::platform::logError("[gfx] unmapped VkFormat %d; colour handling will be wrong\n",
                               static_cast<int>(format));
    return Format::Undefined;
}

VkShaderStageFlags toVk(ShaderStages stages) {
    switch (stages) {
        case ShaderStages::Vertex: return VK_SHADER_STAGE_VERTEX_BIT;
        case ShaderStages::Fragment: return VK_SHADER_STAGE_FRAGMENT_BIT;
        case ShaderStages::Both: return VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    }
    return VK_SHADER_STAGE_VERTEX_BIT;
}

VkBufferUsageFlags toVk(BufferUsage usage) {
    switch (usage) {
        case BufferUsage::Vertex: return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        case BufferUsage::Index: return VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        case BufferUsage::TransferSrc: return VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        case BufferUsage::TransferDst: return VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    }
    return 0;
}

VkFrontFace toVk(Winding winding) {
    return winding == Winding::Clockwise ? VK_FRONT_FACE_CLOCKWISE
                                         : VK_FRONT_FACE_COUNTER_CLOCKWISE;
}

VkFormat toVk(VertexFormat format) {
    switch (format) {
        case VertexFormat::Float2: return VK_FORMAT_R32G32_SFLOAT;
        case VertexFormat::Float3: return VK_FORMAT_R32G32B32_SFLOAT;
        case VertexFormat::Float4: return VK_FORMAT_R32G32B32A32_SFLOAT;
    }
    return VK_FORMAT_UNDEFINED;
}

}
