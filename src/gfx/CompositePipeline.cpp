#include "gfx/CompositePipeline.hpp"

#include "gfx/rhi/Commands.hpp"
#include "gfx/rhi/PipelineBuilder.hpp"

namespace cinder::gfx {

CompositePipeline::CompositePipeline(const cinder::gfx::rhi::Presenter& presenter) {
    pipeline_ = cinder::gfx::rhi::PipelineBuilder(presenter, cinder::gfx::rhi::PassKind::Present)
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
