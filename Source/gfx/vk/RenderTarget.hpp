#pragma once

#include "gfx/rhi/Format.hpp"
#include "gfx/vk/DepthBuffer.hpp"
#include "gfx/vk/TexturePool.hpp"
#include "gfx/vk/VkImages.hpp"

#include <memory>

namespace cinder::gfx::rhi {

class RenderTarget {
public:
    RenderTarget(const cinder::gfx::rhi::Ctx& ctx, VkRenderPass renderPass,
                 Format format, uint32_t width, uint32_t height);
    ~RenderTarget();

    RenderTarget(const RenderTarget&) = delete;
    RenderTarget& operator=(const RenderTarget&) = delete;

    VkFramebuffer framebuffer() const { return framebuffer_; }
    VkImage image() const { return image_.image; }
    VkImageView view() const { return view_; }
    TextureBinding binding() const { return binding_; }
    uint32_t width() const { return width_; }
    uint32_t height() const { return height_; }

private:
    const cinder::gfx::rhi::Ctx& ctx_;
    cinder::gfx::vk::Allocated image_;
    VkImageView view_ = VK_NULL_HANDLE;
    std::unique_ptr<cinder::gfx::vk::DepthBuffer> depth_;
    VkFramebuffer framebuffer_ = VK_NULL_HANDLE;
    std::unique_ptr<TexturePool> pool_;
    TextureBinding binding_;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
};

}
