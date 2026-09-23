#include "gfx/CompositePipeline.hpp"

#include "gfx/vk/GraphicsPipelineBuilder.hpp"

namespace cinder::gfx {

CompositePipeline::CompositePipeline(const cinder::gfx::vk::VkCtx& ctx, VkRenderPass renderPass) {
    pipeline_ = cinder::gfx::vk::GraphicsPipelineBuilder(ctx, renderPass)
                        .shader("composite")
                        .build();
}

void CompositePipeline::draw(VkCommandBuffer cmd, VkDescriptorSet descriptorSet) const {
    pipeline_->bind(cmd);
    pipeline_->bindDescriptorSet(cmd, descriptorSet);
    vkCmdDraw(cmd, FULLSCREEN_TRIANGLE_VERTICES, 1, 0, 0);
}

}
