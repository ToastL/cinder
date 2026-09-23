#pragma once

#include "gfx/rhi/Format.hpp"

#include <volk.h>

namespace cinder::gfx::vk {

class VkCtx;

namespace renderPasses {

VkRenderPass scene(const VkCtx& ctx, rhi::Format colorFormat);
VkRenderPass present(const VkCtx& ctx, rhi::Format colorFormat);

}

}
