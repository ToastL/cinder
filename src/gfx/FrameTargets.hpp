#pragma once

#include "gfx/rhi/Format.hpp"
#include "gfx/RenderTarget.hpp"

#include <memory>
#include <vector>

namespace cinder::gfx {

class FrameTargets {
public:
    void recreate(const cinder::gfx::vk::VkCtx& ctx, VkRenderPass renderPass,
                  cinder::gfx::rhi::Format format, VkExtent2D extent, uint32_t count);
    void clear();
    RenderTarget& at(uint32_t frame) const { return *frames_[frame]; }

private:
    std::vector<std::unique_ptr<RenderTarget>> frames_;
};

}
