#include "Gfx/Pass/SpritePipeline.hpp"

#include "Gfx/RHI/PipelineBuilder.hpp"

namespace cinder::Gfx::Pass {

using cinder::Gfx::RHI::GraphicsPipeline;

SpritePipeline::SpritePipeline(const cinder::Gfx::RHI::Presenter& presenter) {
    pipeline_ = cinder::Gfx::RHI::PipelineBuilder(presenter, cinder::Gfx::RHI::PassKind::Scene)
                        .shader("sprite")
                        .pushConstants(GraphicsPipeline::MATRIX_BYTES)
                        .vertexStride(VERTEX_STRIDE)
                        .attribute(0, cinder::Gfx::RHI::VertexFormat::Float3, 0)
                        .attribute(1, cinder::Gfx::RHI::VertexFormat::Float2, UV_OFFSET)
                        .attribute(2, cinder::Gfx::RHI::VertexFormat::Float4, COLOR_OFFSET)
                        .depthRead()
                        .alphaBlend()
                        .build();
}

void SpritePipeline::bind(cinder::Gfx::RHI::Commands cmd, const glm::mat4& viewProjection) const {
    pipeline_->bind(cmd);
    pipeline_->push(cmd, 0, viewProjection);
}

}
