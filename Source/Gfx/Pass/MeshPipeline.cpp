#include "Gfx/Pass/MeshPipeline.hpp"

#include "Gfx/Asset/Mesh.hpp"
#include "Gfx/RHI/PipelineBuilder.hpp"

namespace cinder::Gfx::Pass {

using cinder::Gfx::Asset::Mesh;
using cinder::Gfx::RHI::GraphicsPipeline;

MeshPipeline::MeshPipeline(const cinder::Gfx::RHI::Presenter& presenter) {
    pipeline_ = cinder::Gfx::RHI::PipelineBuilder(presenter, cinder::Gfx::RHI::PassKind::Scene)
                        .shader("mesh")
                        .pushConstants(2 * GraphicsPipeline::MATRIX_BYTES)
                        .vertexStride(Mesh::VERTEX_STRIDE)
                        .attribute(0, cinder::Gfx::RHI::VertexFormat::Float3, 0)
                        .attribute(1, cinder::Gfx::RHI::VertexFormat::Float3, Mesh::NORMAL_OFFSET)
                        .attribute(2, cinder::Gfx::RHI::VertexFormat::Float2, Mesh::UV_OFFSET)
                        .attribute(3, cinder::Gfx::RHI::VertexFormat::Float4, Mesh::COLOR_OFFSET)
                        .depthTest()
                        .frontFace(cinder::Gfx::RHI::Winding::CounterClockwise)
                        .cullBack()
                        .build();
}

void MeshPipeline::bind(cinder::Gfx::RHI::Commands cmd, const glm::mat4& viewProjection) const {
    pipeline_->bind(cmd);
    pipeline_->push(cmd, 0, viewProjection);
}

void MeshPipeline::pushModel(cinder::Gfx::RHI::Commands cmd, const glm::mat4& model) const {
    pipeline_->push(cmd, GraphicsPipeline::MATRIX_BYTES, model);
}

}
