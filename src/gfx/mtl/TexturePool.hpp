#pragma once

#include "gfx/rhi/Format.hpp"
#include "gfx/rhi/Handles.hpp"

#include <cstdint>
#include <memory>
#include <vector>

namespace cinder::gfx::mtl { struct TextureBindingData; }

namespace cinder::gfx::rhi {

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
    std::vector<std::unique_ptr<cinder::gfx::mtl::TextureBindingData>> bindings_;
};

}
