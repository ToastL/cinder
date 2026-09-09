#pragma once

#include "gfx/vk/GraphicsPipeline.hpp"

#include <memory>

namespace cinder::gfx {

class CompositePipeline {
public:
    CompositePipeline(const cinder::gfx::vk::VkCtx& ctx, VkRenderPass renderPass,
                      VkDescriptorSetLayout descriptorSetLayout);

    void draw(VkCommandBuffer cmd, VkDescriptorSet descriptorSet) const;

private:
    static constexpr uint32_t FULLSCREEN_TRIANGLE_VERTICES = 3;

    std::unique_ptr<cinder::gfx::vk::GraphicsPipeline> pipeline_;
};

}
