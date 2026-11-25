#pragma once

#include "Gfx/RHI/Format.hpp"
#include "Gfx/VK/DepthBuffer.hpp"
#include "Gfx/VK/TexturePool.hpp"
#include "Gfx/VK/VkImages.hpp"

#include <array>
#include <memory>

namespace cinder::Gfx::RHI {

class RenderTarget {
public:
    RenderTarget(const cinder::Gfx::RHI::Ctx& ctx, VkRenderPass scenePass, VkRenderPass resolvePass,
                 Format sceneFormat, Format resolvedFormat, uint32_t width, uint32_t height);
    ~RenderTarget();

    RenderTarget(const RenderTarget&) = delete;
    RenderTarget& operator=(const RenderTarget&) = delete;

    VkFramebuffer framebuffer() const { return framebuffer_; }
    VkFramebuffer overlayFramebuffer() const { return overlayFramebuffer_; }
    VkFramebuffer resolveFramebuffer() const { return resolveFramebuffer_; }
    VkImage resolvedImage() const { return resolved_.image; }
    VkImage depthImage() const { return depth_->image(); }
    TextureBinding binding(uint32_t slot = 0) const { return bindings_[slot]; }
    TextureBinding depthBinding() const { return depthBinding_; }
    TextureBinding resolvedBinding() const { return resolvedBinding_; }
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
    VkFramebuffer overlayFramebuffer_ = VK_NULL_HANDLE;
    cinder::Gfx::VK::Allocated resolved_;
    VkImageView resolvedView_ = VK_NULL_HANDLE;
    VkFramebuffer resolveFramebuffer_ = VK_NULL_HANDLE;
    std::unique_ptr<TexturePool> pool_;
    std::unique_ptr<TexturePool> depthPool_;
    std::unique_ptr<TexturePool> resolvedPool_;
    std::array<TextureBinding, MAX_TEXTURE_SLOTS> bindings_;
    TextureBinding depthBinding_;
    TextureBinding resolvedBinding_;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    mutable bool rendered_ = false;
};

}
