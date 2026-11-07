#pragma once

#include "Gfx/RHI/Format.hpp"
#include "Gfx/VK/DepthBuffer.hpp"
#include "Gfx/VK/TexturePool.hpp"
#include "Gfx/VK/VkImages.hpp"

#include <memory>

namespace cinder::Gfx::RHI {

class RenderTarget {
public:
    RenderTarget(const cinder::Gfx::RHI::Ctx& ctx, VkRenderPass renderPass,
                 Format format, uint32_t width, uint32_t height);
    ~RenderTarget();

    RenderTarget(const RenderTarget&) = delete;
    RenderTarget& operator=(const RenderTarget&) = delete;

    VkFramebuffer framebuffer() const { return framebuffer_; }
    VkImage image() const { return image_.image; }
    TextureBinding binding() const { return binding_; }
    uint32_t width() const { return width_; }
    uint32_t height() const { return height_; }
    bool rendered() const { return rendered_; }
    void markRendered() const { rendered_ = true; }

private:
    const cinder::Gfx::RHI::Ctx& ctx_;
    cinder::Gfx::VK::Allocated image_;
    VkImageView view_ = VK_NULL_HANDLE;
    std::unique_ptr<cinder::Gfx::VK::DepthBuffer> depth_;
    VkFramebuffer framebuffer_ = VK_NULL_HANDLE;
    std::unique_ptr<TexturePool> pool_;
    TextureBinding binding_;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    mutable bool rendered_ = false;
};

}
