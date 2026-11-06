#pragma once

#include "gfx/rhi/Format.hpp"

#include <volk.h>

namespace cinder::gfx::rhi {

VkFormat toVk(Format format);
VkFormat toVk(VertexFormat format);
VkShaderStageFlags toVk(ShaderStages stages);
VkFrontFace toVk(Winding winding);
VkBufferUsageFlags toVk(BufferUsage usage);
Format fromVk(VkFormat format);

}
