#pragma once

#include "Gfx/RHI/Format.hpp"

#include <volk.h>

namespace cinder::Gfx::RHI {

VkFormat toVk(Format format);
VkFormat toVk(VertexFormat format);
VkShaderStageFlags toVk(ShaderStages stages);
VkFrontFace toVk(Winding winding);
VkBufferUsageFlags toVk(BufferUsage usage);
Format fromVk(VkFormat format);
bool knownVk(VkFormat format);

}
