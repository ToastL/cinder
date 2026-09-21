#include "gfx/FrameTargets.hpp"

namespace cinder::gfx {

void FrameTargets::recreate(const cinder::gfx::vk::VkCtx& ctx, VkRenderPass renderPass,
                            VkDescriptorSetLayout textureLayout, VkFormat format, VkFormat depthFormat,
                            VkExtent2D extent, uint32_t count, Overlay* overlay) {
    clear();
    frames_.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        auto target = std::make_unique<RenderTarget>(ctx, renderPass, textureLayout, format, depthFormat,
                                                    extent.width, extent.height);
        OverlayTexture texture;
        if (overlay != nullptr) texture = OverlayTexture(*overlay, target->view());
        frames_.push_back({std::move(target), std::move(texture)});
    }
}

void FrameTargets::clear() { frames_.clear(); }

VkDescriptorSet FrameTargets::viewport(uint32_t frame) const {
    return frames_.empty() ? VK_NULL_HANDLE : frames_[frame].texture.handle();
}

}
