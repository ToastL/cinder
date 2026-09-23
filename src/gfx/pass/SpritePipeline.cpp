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
                        .attribute(0, VK_FORMAT_R32G32B32_SFLOAT, 0)
                        .attribute(1, VK_FORMAT_R32G32_SFLOAT, 3 * sizeof(float))
                        .attribute(2, VK_FORMAT_R32G32B32A32_SFLOAT, 5 * sizeof(float))
                        .depthRead()
                        .alphaBlend()
                        .build();
}

void SpritePipeline::bind(VkCommandBuffer cmd, const glm::mat4& viewProjection) const {
    pipeline_->bind(cmd);
    pipeline_->push(cmd, 0, viewProjection);
}

}
