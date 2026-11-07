#pragma once

#include "Gfx/Pass/DrawPass.hpp"
#include "Gfx/Pass/SpriteBatch.hpp"

namespace cinder::Gfx::Pass {

class SpritePass : public DrawPass {
public:
    SpritePass(const cinder::Gfx::RHI::Ctx& ctx, cinder::Gfx::Asset::Assets& assets,
               const SpritePipeline& pipeline, uint32_t framesInFlight);

    void beginFrame() override;
    void record(cinder::Gfx::RHI::Commands cmd, uint32_t frameInFlight,
                const glm::mat4& viewProjection) override;

    void draw(int texture, const glm::mat4& model, glm::vec2 size, const glm::vec4& color);
    void drawRegion(int texture, const glm::mat4& model, glm::vec2 size, const glm::vec4& region,
                    const glm::vec4& color);

private:
    SpriteBatch batch_;
};

}
