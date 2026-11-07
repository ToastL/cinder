#include "Gfx/VK/VkUtil.hpp"

#include <stdexcept>
#include <string>

namespace cinder::Gfx::VK {

void check(VkResult result, std::string_view what) {
    if (result == VK_SUCCESS) return;
    throw std::runtime_error(std::string(what) + " failed: VkResult " + std::to_string(result));
}

}
