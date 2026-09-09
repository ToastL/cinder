#pragma once

#include "gfx/vk/GraphicsPipeline.hpp"

#include <memory>

namespace cinder::gfx::pass {

class MeshPipeline {
public:
    MeshPipeline(const cinder::gfx::vk::VkCtx& ctx, VkRenderPass renderPass,
                 VkDescriptorSetLayout descriptorSetLayout);

    void bind(VkCommandBuffer cmd, const glm::mat4& viewProjection) const;
    void pushModel(VkCommandBuffer cmd, const glm::mat4& model) const;
    VkPipelineLayout layout() const { return pipeline_->layout(); }

private:
    std::unique_ptr<cinder::gfx::vk::GraphicsPipeline> pipeline_;
};

}
