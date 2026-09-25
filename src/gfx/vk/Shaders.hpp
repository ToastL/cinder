#pragma once

#include <volk.h>

#include <string_view>

namespace cinder::gfx::rhi { class Ctx; }

namespace cinder::gfx::vk {

namespace shaders {

VkShaderModule fromFile(const rhi::Ctx& ctx, std::string_view name);

}

}
