#pragma once

#include "Gfx/VK/Loader.hpp"

#include <string_view>

namespace cinder::Gfx::VK {

void check(VkResult result, std::string_view what);

}
