#pragma once

#include "Gfx/VK/Loader.hpp"

#include <string_view>

namespace cinder::Gfx::RHI { class Ctx; }

namespace cinder::Gfx::VK {

namespace shaders {

VkShaderModule fromFile(const RHI::Ctx& ctx, std::string_view name);

}

}
