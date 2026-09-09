#pragma once

#include <volk.h>

#include <string_view>

namespace cinder::gfx::vk {

class VkCtx;

namespace shaders {

VkShaderModule fromFile(const VkCtx& ctx, std::string_view name);

}

}
