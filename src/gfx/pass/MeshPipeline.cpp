#include "gfx/pass/MeshPipeline.hpp"

#include "gfx/asset/Mesh.hpp"
#include "gfx/vk/GraphicsPipelineBuilder.hpp"

namespace cinder::gfx::pass {

using cinder::gfx::asset::Mesh;
using cinder::gfx::vk::GraphicsPipeline;
using cinder::gfx::vk::GraphicsPipelineBuilder;

MeshPipeline::MeshPipeline(const cinder::gfx::vk::VkCtx& ctx, VkRenderPass renderPass) {
    pipeline_ = GraphicsPipelineBuilder(ctx, renderPass)
                        .shader("mesh")
                        .pushConstants(2 * GraphicsPipeline::MATRIX_BYTES)
                        .vertexStride(Mesh::VERTEX_STRIDE)
                        .attribute(0, VK_FORMAT_R32G32B32_SFLOAT, 0)
                        .attribute(1, VK_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float))
                        .attribute(2, VK_FORMAT_R32G32_SFLOAT, 6 * sizeof(float))
                        .attribute(3, VK_FORMAT_R32G32B32A32_SFLOAT, 8 * sizeof(float))
                        .depthTest()
                        .frontFace(VK_FRONT_FACE_CLOCKWISE)
                        .build();
}

void MeshPipeline::bind(cinder::gfx::rhi::Commands cmd, const glm::mat4& viewProjection) const {
    pipeline_->bind(cmd);
    pipeline_->push(cmd, 0, viewProjection);
}

void MeshPipeline::pushModel(cinder::gfx::rhi::Commands cmd, const glm::mat4& model) const {
    pipeline_->push(cmd, GraphicsPipeline::MATRIX_BYTES, model);
}

}
