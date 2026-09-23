#include "gfx/pass/MeshPipeline.hpp"

#include "gfx/asset/Mesh.hpp"
#include "gfx/rhi/PipelineBuilder.hpp"

namespace cinder::gfx::pass {

using cinder::gfx::asset::Mesh;
using cinder::gfx::rhi::GraphicsPipeline;

MeshPipeline::MeshPipeline(const cinder::gfx::rhi::Presenter& presenter) {
    pipeline_ = cinder::gfx::rhi::PipelineBuilder(presenter, cinder::gfx::rhi::PassKind::Scene)
                        .shader("mesh")
                        .pushConstants(2 * GraphicsPipeline::MATRIX_BYTES)
                        .vertexStride(Mesh::VERTEX_STRIDE)
                        .attribute(0, cinder::gfx::rhi::VertexFormat::Float3, 0)
                        .attribute(1, cinder::gfx::rhi::VertexFormat::Float3, 3 * sizeof(float))
                        .attribute(2, cinder::gfx::rhi::VertexFormat::Float2, 6 * sizeof(float))
                        .attribute(3, cinder::gfx::rhi::VertexFormat::Float4, 8 * sizeof(float))
                        .depthTest()
                        .frontFace(cinder::gfx::rhi::Winding::Clockwise)
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
