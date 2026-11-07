#pragma once

#include "Gfx/RHI/Handles.hpp"

#include <cstdint>

namespace cinder::Gfx::MTL {

struct TextureBindingData {
    void* texture = nullptr;
    void* sampler = nullptr;
};

}

namespace cinder::Gfx::RHI {

inline cinder::Gfx::MTL::TextureBindingData* unwrap(TextureBinding binding) {
    return reinterpret_cast<cinder::Gfx::MTL::TextureBindingData*>(binding.id);
}

inline TextureBinding binding(cinder::Gfx::MTL::TextureBindingData* value) {
    return TextureBinding{reinterpret_cast<std::uint64_t>(value)};
}

}
