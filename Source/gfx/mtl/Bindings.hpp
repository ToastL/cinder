#pragma once

#include "gfx/rhi/Handles.hpp"

#include <cstdint>

namespace cinder::gfx::mtl {

struct TextureBindingData {
    void* texture = nullptr;
    void* sampler = nullptr;
};

}

namespace cinder::gfx::rhi {

inline cinder::gfx::mtl::TextureBindingData* unwrap(TextureBinding binding) {
    return reinterpret_cast<cinder::gfx::mtl::TextureBindingData*>(binding.id);
}

inline TextureBinding binding(cinder::gfx::mtl::TextureBindingData* value) {
    return TextureBinding{reinterpret_cast<std::uint64_t>(value)};
}

}
