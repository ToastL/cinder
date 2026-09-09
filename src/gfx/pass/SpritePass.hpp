#pragma once

#include "gfx/pass/DrawPass.hpp"
#include "gfx/pass/OrthographicCamera.hpp"
#include "gfx/pass/SpriteBatch.hpp"

namespace cinder::gfx::pass {

class SpritePass : public DrawPass {
public:
    SpritePass(const cinder::gfx::vk::VkCtx& ctx, cinder::gfx::asset::Assets& assets,
               const SpritePipeline& pipeline, uint32_t framesInFlight);

    void beginFrame() override;
    void record(VkCommandBuffer cmd, uint32_t frameInFlight) override;
    void registerApi(cinder::lua::LuaApi& api) override;
    void resize(int width, int height) override;

    void draw(int texture, float x, float y, float w, float h, float rot,
              float r, float g, float b, float a);
    void drawRegion(int texture, float x, float y, float w, float h,
                    float sx, float sy, float sw, float sh, float rot,
                    float r, float g, float b, float a);

    OrthographicCamera& camera() { return camera_; }
    void setVirtualSize(float width, float height);
    glm::vec3 screenToWorld(float screenX, float screenY);

private:
    SpriteBatch batch_;
    OrthographicCamera camera_;
    float viewWidth_ = 1.0f;
    float viewHeight_ = 1.0f;
    bool virtualSizeExplicit_ = false;
};

}
