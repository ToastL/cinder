#include "gfx/FrameTargets.hpp"

namespace cinder::gfx {

void FrameTargets::recreate(const cinder::gfx::vk::VkCtx& ctx, VkRenderPass renderPass,
                            cinder::gfx::rhi::Format format, VkExtent2D extent,
                            uint32_t count) {
    clear();
    frames_.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        frames_.push_back(std::make_unique<cinder::gfx::rhi::RenderTarget>(ctx, renderPass, format,
                                                         extent.width, extent.height));
    }
}

void FrameTargets::clear() { frames_.clear(); }

}
