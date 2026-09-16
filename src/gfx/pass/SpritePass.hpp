#pragma once

#include "gfx/pass/DrawPass.hpp"
#include "gfx/pass/SpriteBatch.hpp"

namespace cinder::gfx::pass {

class SpritePass : public DrawPass {
public:
    SpritePass(const cinder::gfx::vk::VkCtx& ctx, cinder::gfx::asset::Assets& assets,
               const SpritePipeline& pipeline, uint32_t framesInFlight);

    void beginFrame() override;
    void record(VkCommandBuffer cmd, uint32_t frameInFlight, const glm::mat4& viewProjection) override;
    void registerApi(cinder::lua::LuaApi& api) override;

    void draw(int texture, const glm::mat4& model, glm::vec2 size, const glm::vec4& color);
    void drawRegion(int texture, const glm::mat4& model, glm::vec2 size, const glm::vec4& region,
                    const glm::vec4& color);

private:
    SpriteBatch batch_;
};

}
