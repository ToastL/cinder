#include "gfx/pass/SpritePipeline.hpp"

#include "gfx/vk/GraphicsPipelineBuilder.hpp"

namespace cinder::gfx::pass {

using cinder::gfx::vk::GraphicsPipeline;
using cinder::gfx::vk::GraphicsPipelineBuilder;

SpritePipeline::SpritePipeline(const cinder::gfx::vk::VkCtx& ctx, VkRenderPass renderPass) {
    pipeline_ = GraphicsPipelineBuilder(ctx, renderPass)
                        .shader("sprite")
                        .pushConstants(GraphicsPipeline::MATRIX_BYTES)
                        .vertexStride(VERTEX_STRIDE)
                        .attribute(0, cinder::gfx::rhi::VertexFormat::Float3, 0)
                        .attribute(1, cinder::gfx::rhi::VertexFormat::Float2, 3 * sizeof(float))
                        .attribute(2, cinder::gfx::rhi::VertexFormat::Float4, 5 * sizeof(float))
                        .depthRead()
                        .alphaBlend()
                        .build();
}

void SpritePipeline::bind(cinder::gfx::rhi::Commands cmd, const glm::mat4& viewProjection) const {
    pipeline_->bind(cmd);
    pipeline_->push(cmd, 0, viewProjection);
}

}
