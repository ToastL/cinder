#pragma once

#include "Gfx/RHI/Handles.hpp"

#include <volk.h>

#include <cstdint>

namespace cinder::Gfx::RHI {

inline VkDescriptorSet unwrap(TextureBinding binding) {
    return reinterpret_cast<VkDescriptorSet>(binding.id);
}

inline TextureBinding binding(VkDescriptorSet set) {
    return TextureBinding{reinterpret_cast<std::uint64_t>(set)};
}

}
