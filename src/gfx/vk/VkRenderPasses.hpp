#pragma once

#include "gfx/rhi/Format.hpp"

#include <volk.h>

namespace cinder::gfx::rhi { class Ctx; }

namespace cinder::gfx::vk {

namespace renderPasses {

VkRenderPass scene(const rhi::Ctx& ctx, rhi::Format colorFormat);
VkRenderPass present(const rhi::Ctx& ctx, rhi::Format colorFormat);

}

}
