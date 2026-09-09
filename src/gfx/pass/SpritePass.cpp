#include "gfx/pass/SpritePass.hpp"

#include "gfx/pass/SpriteApi.hpp"

namespace cinder::gfx::pass {

SpritePass::SpritePass(const cinder::gfx::vk::VkCtx& ctx, cinder::gfx::asset::Assets& assets,
                       const SpritePipeline& pipeline, uint32_t framesInFlight)
    : batch_(ctx, assets, pipeline, framesInFlight) {}

void SpritePass::beginFrame() { batch_.reset(); }

void SpritePass::record(VkCommandBuffer cmd, uint32_t frameInFlight) {
    batch_.flush(cmd, frameInFlight, camera_.viewProjection());
}

void SpritePass::registerApi(cinder::lua::LuaApi& api) { registerSpriteApi(api, *this); }

void SpritePass::resize(int width, int height) {
    viewWidth_ = static_cast<float>(width);
    viewHeight_ = static_cast<float>(height);
    if (!virtualSizeExplicit_) camera_.setVirtualSize(viewWidth_, viewHeight_);
}

void SpritePass::draw(int texture, float x, float y, float w, float h, float rot,
                      float r, float g, float b, float a) {
    batch_.draw(texture, x, y, w, h, rot, r, g, b, a);
}

void SpritePass::drawRegion(int texture, float x, float y, float w, float h,
                            float sx, float sy, float sw, float sh, float rot,
                            float r, float g, float b, float a) {
    batch_.drawRegion(texture, x, y, w, h, sx, sy, sw, sh, rot, r, g, b, a);
}

void SpritePass::setVirtualSize(float width, float height) {
    virtualSizeExplicit_ = true;
    camera_.setVirtualSize(width, height);
}

glm::vec3 SpritePass::screenToWorld(float screenX, float screenY) {
    return camera_.screenToWorld(screenX, screenY, viewWidth_, viewHeight_);
}

}
