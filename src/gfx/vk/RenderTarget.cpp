#include "gfx/vk/RenderTarget.hpp"

#include "gfx/vk/Formats.hpp"
#include "gfx/vk/VkCtx.hpp"

namespace cinder::gfx::rhi {

using cinder::gfx::vk::DepthBuffer;
using cinder::gfx::vk::VkCtx;
namespace images = cinder::gfx::vk::images;

RenderTarget::RenderTarget(const VkCtx& ctx, VkRenderPass renderPass,
                           Format format, uint32_t width, uint32_t height)
    : ctx_(ctx), width_(width), height_(height) {
    const VkFormat color = toVk(format);
    image_ = images::create(ctx, color, width, height,
                            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
                                    | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    view_ = images::view(ctx, image_.image, color, VK_IMAGE_ASPECT_COLOR_BIT);

    depth_ = std::make_unique<DepthBuffer>(ctx, toVk(ctx.depthFormat()),
                                          width, height);

    const VkImageView attachments[] = {view_, depth_->view()};
    framebuffer_ = images::framebuffer(ctx, renderPass, attachments, 2, width, height);

    pool_ = std::make_unique<TexturePool>(ctx, 1, VK_FILTER_LINEAR);
    binding_ = pool_->bind(view_);
}

RenderTarget::~RenderTarget() {
    pool_.reset();
    vkDestroyFramebuffer(ctx_.device(), framebuffer_, nullptr);
    depth_.reset();
    vkDestroyImageView(ctx_.device(), view_, nullptr);
    vmaDestroyImage(ctx_.allocator(), image_.image, image_.allocation);
}

}
