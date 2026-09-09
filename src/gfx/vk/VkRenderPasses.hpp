#pragma once

#include <volk.h>

namespace cinder::gfx::vk {

class VkCtx;

namespace renderPasses {

VkRenderPass scene(const VkCtx& ctx, VkFormat colorFormat, VkFormat depthFormat);
VkRenderPass present(const VkCtx& ctx, VkFormat colorFormat);

}

}
