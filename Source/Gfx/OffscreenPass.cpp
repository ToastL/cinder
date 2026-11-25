#include "Gfx/OffscreenPass.hpp"

#include "Gfx/RHI/Backend.hpp"
#include "Gfx/RHI/Commands.hpp"

namespace cinder::Gfx {

OffscreenPass::OffscreenPass(cinder::Gfx::RHI::Presenter& presenter, cinder::Gfx::RHI::Frame& frame,
                             const glm::mat4& viewProjection, cinder::Gfx::RHI::TextureBinding sceneDepth,
                             glm::uvec2 sceneSize)
    : presenter_(presenter), frame_(frame), viewProjection_(viewProjection), sceneDepth_(sceneDepth),
      sceneSize_(sceneSize) {}

cinder::Gfx::RHI::Commands OffscreenPass::commands() const { return frame_.commands; }

std::uint32_t OffscreenPass::frameInFlight() const { return frame_.index; }

void OffscreenPass::begin(const cinder::Gfx::RHI::RenderTarget& target) {
    presenter_.beginScenePass(frame_, target, 0.0f, 0.0f, 0.0f);
    cinder::Gfx::RHI::viewport(frame_.commands, target.width(), target.height());
    cinder::Gfx::RHI::scissor(frame_.commands, 0, 0, target.width(), target.height());
}

void OffscreenPass::end() { presenter_.endPass(frame_); }

}
