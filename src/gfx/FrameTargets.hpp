#pragma once

#include "gfx/Overlay.hpp"
#include "gfx/RenderTarget.hpp"

#include <memory>
#include <vector>

namespace cinder::gfx {

class FrameTargets {
public:
    void recreate(const cinder::gfx::vk::VkCtx& ctx, VkRenderPass renderPass,
                  VkDescriptorSetLayout textureLayout, VkFormat format, VkFormat depthFormat,
                  VkExtent2D extent, uint32_t count, Overlay* overlay);
    void clear();
    RenderTarget& at(uint32_t frame) const { return *frames_[frame].target; }
    VkDescriptorSet viewport(uint32_t frame) const;

private:
    struct Frame {
        std::unique_ptr<RenderTarget> target;
        OverlayTexture texture;
    };

    std::vector<Frame> frames_;
};

}
