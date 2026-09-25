#pragma once

#include "gfx/rhi/Handles.hpp"

#include <volk.h>

#include <cstdint>

namespace cinder::gfx::rhi {

inline VkDescriptorSet unwrap(TextureBinding binding) {
    return reinterpret_cast<VkDescriptorSet>(binding.id);
}

inline TextureBinding binding(VkDescriptorSet set) {
    return TextureBinding{reinterpret_cast<std::uint64_t>(set)};
}

}
