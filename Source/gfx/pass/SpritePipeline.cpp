#include "gfx/pass/SpritePipeline.hpp"

#include "gfx/rhi/PipelineBuilder.hpp"

namespace cinder::gfx::pass {

using cinder::gfx::rhi::GraphicsPipeline;

SpritePipeline::SpritePipeline(const cinder::gfx::rhi::Presenter& presenter) {
    pipeline_ = cinder::gfx::rhi::PipelineBuilder(presenter, cinder::gfx::rhi::PassKind::Scene)
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
