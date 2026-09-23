#include "gfx/CompositePipeline.hpp"

#include "gfx/rhi/Commands.hpp"

#include "gfx/vk/GraphicsPipelineBuilder.hpp"

namespace cinder::gfx {

CompositePipeline::CompositePipeline(const cinder::gfx::vk::VkCtx& ctx, VkRenderPass renderPass) {
    pipeline_ = cinder::gfx::vk::GraphicsPipelineBuilder(ctx, renderPass)
                        .shader("composite")
                        .build();
}

void CompositePipeline::draw(cinder::gfx::rhi::Commands cmd,
                             cinder::gfx::rhi::TextureBinding texture) const {
    pipeline_->bind(cmd);
    pipeline_->bindTexture(cmd, texture);
    cinder::gfx::rhi::draw(cmd, FULLSCREEN_TRIANGLE_VERTICES);
}

}
