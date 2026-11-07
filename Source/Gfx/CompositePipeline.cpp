#include "Gfx/CompositePipeline.hpp"

#include "Gfx/RHI/Commands.hpp"
#include "Gfx/RHI/PipelineBuilder.hpp"

namespace cinder::Gfx {

CompositePipeline::CompositePipeline(const cinder::Gfx::RHI::Presenter& presenter) {
    pipeline_ = cinder::Gfx::RHI::PipelineBuilder(presenter, cinder::Gfx::RHI::PassKind::Present)
                        .shader("composite")
                        .build();
}

void CompositePipeline::draw(cinder::Gfx::RHI::Commands cmd,
                             cinder::Gfx::RHI::TextureBinding texture) const {
    pipeline_->bind(cmd);
    pipeline_->bindTexture(cmd, texture);
    cinder::Gfx::RHI::draw(cmd, FULLSCREEN_TRIANGLE_VERTICES);
}

}
