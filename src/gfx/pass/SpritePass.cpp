#include "gfx/pass/SpritePass.hpp"

#include "gfx/pass/SpriteApi.hpp"

namespace cinder::gfx::pass {

SpritePass::SpritePass(const cinder::gfx::vk::VkCtx& ctx, cinder::gfx::asset::Assets& assets,
                       const SpritePipeline& pipeline, uint32_t framesInFlight)
    : batch_(ctx, assets, pipeline, framesInFlight) {}

void SpritePass::beginFrame() { batch_.reset(); }

void SpritePass::record(VkCommandBuffer cmd, uint32_t frameInFlight, const glm::mat4& viewProjection) {
    batch_.flush(cmd, frameInFlight, viewProjection);
}

void SpritePass::registerApi(cinder::lua::LuaApi& api) { registerSpriteApi(api, *this); }

void SpritePass::draw(int texture, const glm::mat4& model, glm::vec2 size, const glm::vec4& color) {
    batch_.draw(texture, model, size, color);
}

void SpritePass::drawRegion(int texture, const glm::mat4& model, glm::vec2 size,
                            const glm::vec4& region, const glm::vec4& color) {
    batch_.drawRegion(texture, model, size, region, color);
}

}
