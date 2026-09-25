#pragma once

#include "gfx/rhi/Format.hpp"
#include "gfx/rhi/Handles.hpp"

#include <cstdint>
#include <memory>

namespace cinder::gfx::rhi {

class Ctx;
class TexturePool;

class RenderTarget {
public:
    RenderTarget(const Ctx& ctx, Format format, std::uint32_t width, std::uint32_t height);
    ~RenderTarget();

    RenderTarget(const RenderTarget&) = delete;
    RenderTarget& operator=(const RenderTarget&) = delete;

    void* color() const { return color_; }
    void* depth() const { return depth_; }
    TextureBinding binding() const { return binding_; }
    std::uint32_t width() const { return width_; }
    std::uint32_t height() const { return height_; }

private:
    void* color_ = nullptr;
    void* depth_ = nullptr;
    std::unique_ptr<TexturePool> pool_;
    TextureBinding binding_;
    std::uint32_t width_ = 0;
    std::uint32_t height_ = 0;
};

}
