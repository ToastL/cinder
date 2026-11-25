#pragma once

#include "Gfx/RHI/Format.hpp"
#include "Gfx/RHI/Handles.hpp"

#include <cstdint>
#include <memory>

namespace cinder::Gfx::RHI {

class Ctx;
class TexturePool;

class RenderTarget {
public:
    RenderTarget(const Ctx& ctx, Format sceneFormat, Format resolvedFormat, std::uint32_t width,
                 std::uint32_t height);
    ~RenderTarget();

    RenderTarget(const RenderTarget&) = delete;
    RenderTarget& operator=(const RenderTarget&) = delete;

    void* color() const { return color_; }
    void* depth() const { return depth_; }
    void* resolved() const { return resolved_; }
    TextureBinding binding() const { return binding_; }
    TextureBinding depthBinding() const { return depthBinding_; }
    TextureBinding resolvedBinding() const { return resolvedBinding_; }
    std::uint32_t width() const { return width_; }
    std::uint32_t height() const { return height_; }

private:
    void* color_ = nullptr;
    void* depth_ = nullptr;
    void* resolved_ = nullptr;
    std::unique_ptr<TexturePool> pool_;
    std::unique_ptr<TexturePool> depthPool_;
    std::unique_ptr<TexturePool> resolvedPool_;
    TextureBinding binding_;
    TextureBinding depthBinding_;
    TextureBinding resolvedBinding_;
    std::uint32_t width_ = 0;
    std::uint32_t height_ = 0;
};

}
