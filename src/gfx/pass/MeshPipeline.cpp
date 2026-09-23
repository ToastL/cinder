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
                        .attribute(0, cinder::gfx::rhi::VertexFormat::Float3, 0)
                        .attribute(1, cinder::gfx::rhi::VertexFormat::Float3, 3 * sizeof(float))
                        .attribute(2, cinder::gfx::rhi::VertexFormat::Float2, 6 * sizeof(float))
                        .attribute(3, cinder::gfx::rhi::VertexFormat::Float4, 8 * sizeof(float))
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
