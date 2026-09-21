#pragma once

#include <volk.h>

#include <string>

namespace cinder::gfx::vk { class VkCtx; }

namespace cinder::gfx {

class RenderTarget;

void captureTarget(const cinder::gfx::vk::VkCtx& ctx, const RenderTarget& target,
                   VkFormat format, const std::string& path);

}
