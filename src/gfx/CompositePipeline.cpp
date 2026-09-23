#include "gfx/CompositePipeline.hpp"

#include "gfx/rhi/Commands.hpp"

#include "gfx/vk/GraphicsPipelineBuilder.hpp"

namespace cinder::gfx {

CompositePipeline::CompositePipeline(const cinder::gfx::vk::VkCtx& ctx, VkRenderPass renderPass) {
    pipeline_ = cinder::gfx::vk::GraphicsPipelineBuilder(ctx, renderPass)
                        .shader("composite")
                        .build();
}

void CompositePipeline::draw(cinder::gfx::rhi::Commands cmd, VkDescriptorSet descriptorSet) const {
    pipeline_->bind(cmd);
    pipeline_->bindDescriptorSet(cmd, descriptorSet);
    cinder::gfx::rhi::draw(cmd, FULLSCREEN_TRIANGLE_VERTICES);
}

}
