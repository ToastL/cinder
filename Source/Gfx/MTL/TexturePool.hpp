#pragma once

#include "Gfx/RHI/Format.hpp"
#include "Gfx/RHI/Handles.hpp"

#include <cstdint>
#include <memory>
#include <vector>

namespace cinder::Gfx::MTL { struct TextureBindingData; }

namespace cinder::Gfx::RHI {

class Ctx;
class Texture;

class TexturePool {
public:
    TexturePool(const Ctx& ctx, std::uint32_t maxSets, SamplerFilter filter);
    ~TexturePool();

    TexturePool(const TexturePool&) = delete;
    TexturePool& operator=(const TexturePool&) = delete;

    TextureBinding bind(void* texture);
    TextureBinding bind(const Texture& texture);

private:
    std::uint32_t maxSets_ = 0;
    void* sampler_ = nullptr;
    std::vector<std::unique_ptr<cinder::Gfx::MTL::TextureBindingData>> bindings_;
};

}
