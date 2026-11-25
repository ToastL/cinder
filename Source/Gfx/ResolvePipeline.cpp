#include "Gfx/ResolvePipeline.hpp"

#include "Gfx/RHI/Commands.hpp"
#include "Gfx/RHI/PipelineBuilder.hpp"

namespace cinder::Gfx {

namespace {

struct Push {
    float exposure;
};

}

ResolvePipeline::ResolvePipeline(const cinder::Gfx::RHI::Presenter& presenter) {
    pipeline_ = cinder::Gfx::RHI::PipelineBuilder(presenter, cinder::Gfx::RHI::PassKind::Resolve)
                        .shader("resolve")
                        .pushConstants(sizeof(Push), cinder::Gfx::RHI::ShaderStages::Fragment)
                        .build();
}

void ResolvePipeline::draw(cinder::Gfx::RHI::Commands cmd, cinder::Gfx::RHI::TextureBinding scene,
                           float exposure) const {
    const Push push{exposure};
    pipeline_->bind(cmd);
    pipeline_->push(cmd, cinder::Gfx::RHI::ShaderStages::Fragment, sizeof(Push), &push);
    pipeline_->bindTexture(cmd, scene);
    cinder::Gfx::RHI::draw(cmd, FULLSCREEN_TRIANGLE_VERTICES);
}

}
