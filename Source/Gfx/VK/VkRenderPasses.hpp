#pragma once

#include "Gfx/RHI/Format.hpp"

#include <volk.h>

namespace cinder::Gfx::RHI { class Ctx; }

namespace cinder::Gfx::VK {

namespace renderPasses {

VkRenderPass scene(const RHI::Ctx& ctx, RHI::Format colorFormat);
VkRenderPass present(const RHI::Ctx& ctx, RHI::Format colorFormat);

}

}
