#pragma once

#include "gfx/vk/GraphicsPipeline.hpp"

#include <memory>

namespace cinder::gfx::pass {

class SpritePipeline {
public:
    static constexpr uint32_t FLOATS_PER_VERTEX = 8;
    static constexpr uint32_t VERTEX_STRIDE = FLOATS_PER_VERTEX * sizeof(float);

    SpritePipeline(const cinder::gfx::vk::VkCtx& ctx, VkRenderPass renderPass,
                   VkDescriptorSetLayout descriptorSetLayout);

    void bind(VkCommandBuffer cmd, const glm::mat4& projection) const;
    VkPipelineLayout layout() const { return pipeline_->layout(); }

private:
    std::unique_ptr<cinder::gfx::vk::GraphicsPipeline> pipeline_;
};

}
