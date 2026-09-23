#include "gfx/FrameTargets.hpp"

namespace cinder::gfx {

void FrameTargets::recreate(const cinder::gfx::vk::VkCtx& ctx, VkRenderPass renderPass,
                            VkDescriptorSetLayout textureLayout, VkFormat format, VkFormat depthFormat,
                            VkExtent2D extent, uint32_t count) {
    clear();
    frames_.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        frames_.push_back(std::make_unique<RenderTarget>(ctx, renderPass, textureLayout, format, depthFormat,
                                                         extent.width, extent.height));
    }
}

void FrameTargets::clear() { frames_.clear(); }

}
