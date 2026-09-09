#include "gfx/RenderTarget.hpp"

#include "gfx/vk/VkCtx.hpp"
#include "gfx/vk/VkDescriptors.hpp"

namespace cinder::gfx {

using cinder::gfx::vk::DepthBuffer;
using cinder::gfx::vk::VkCtx;
namespace descriptors = cinder::gfx::vk::descriptors;
namespace images = cinder::gfx::vk::images;

RenderTarget::RenderTarget(const VkCtx& ctx, VkRenderPass renderPass,
                           VkDescriptorSetLayout descriptorSetLayout,
                           VkFormat format, VkFormat depthFormat,
                           uint32_t width, uint32_t height)
    : ctx_(ctx), width_(width), height_(height) {
    image_ = images::create(ctx, format, width, height,
                            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
                                    | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    view_ = images::view(ctx, image_.image, format, VK_IMAGE_ASPECT_COLOR_BIT);

    depth_ = std::make_unique<DepthBuffer>(ctx, depthFormat, width, height);

    const VkImageView attachments[] = {view_, depth_->view()};
    framebuffer_ = images::framebuffer(ctx, renderPass, attachments, 2, width, height);

    sampler_ = images::sampler(ctx, VK_FILTER_LINEAR);
    pool_ = descriptors::pool(ctx, 1);
    descriptorSet_ = descriptors::allocate(ctx, pool_, descriptorSetLayout);
    descriptors::writeCombinedImageSampler(ctx, descriptorSet_, view_, sampler_);
}

RenderTarget::~RenderTarget() {
    vkDestroyDescriptorPool(ctx_.device(), pool_, nullptr);
    vkDestroySampler(ctx_.device(), sampler_, nullptr);
    vkDestroyFramebuffer(ctx_.device(), framebuffer_, nullptr);
    depth_.reset();
    vkDestroyImageView(ctx_.device(), view_, nullptr);
    vmaDestroyImage(ctx_.allocator(), image_.image, image_.allocation);
}

}
