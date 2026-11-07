#include "Gfx/Pass/SpritePass.hpp"

namespace cinder::Gfx::Pass {

SpritePass::SpritePass(const cinder::Gfx::RHI::Ctx& ctx, cinder::Gfx::Asset::Assets& assets,
                       const SpritePipeline& pipeline, uint32_t framesInFlight)
    : batch_(ctx, assets, pipeline, framesInFlight) {}

void SpritePass::beginFrame() { batch_.reset(); }

void SpritePass::record(cinder::Gfx::RHI::Commands cmd, uint32_t frameInFlight,
                        const glm::mat4& viewProjection) {
    batch_.flush(cmd, frameInFlight, viewProjection);
}

void SpritePass::draw(int texture, const glm::mat4& model, glm::vec2 size, const glm::vec4& color) {
    batch_.draw(texture, model, size, color);
}

void SpritePass::drawRegion(int texture, const glm::mat4& model, glm::vec2 size,
                            const glm::vec4& region, const glm::vec4& color) {
    batch_.drawRegion(texture, model, size, region, color);
}

}
