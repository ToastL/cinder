#pragma once

#include <volk.h>

#include <string_view>

namespace cinder::gfx::vk {

void check(VkResult result, std::string_view what);

}
