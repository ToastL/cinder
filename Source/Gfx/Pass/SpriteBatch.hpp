#pragma once

#include "Gfx/RHI/Handles.hpp"

#include "Gfx/Asset/Assets.hpp"
#include "Gfx/Pass/SpritePipeline.hpp"
#include "Gfx/RHI/Backend.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <memory>
#include <vector>

namespace cinder::Gfx::Pass {

class SpriteBatch {
public:
    static constexpr uint32_t MAX_QUADS = 10000;
    static constexpr uint32_t FLOATS_PER_QUAD = 4 * SpritePipeline::FLOATS_PER_VERTEX;

    SpriteBatch(const cinder::Gfx::RHI::Ctx& ctx, cinder::Gfx::Asset::Assets& assets,
                const SpritePipeline& pipeline, uint32_t framesInFlight);

    void reset() { quadCount_ = 0; }

    void draw(int texture, const glm::mat4& model, glm::vec2 size, const glm::vec4& color);
    void drawRegion(int texture, const glm::mat4& model, glm::vec2 size, const glm::vec4& region,
                    const glm::vec4& color);

    void flush(cinder::Gfx::RHI::Commands cmd, uint32_t frameIndex,
               const glm::mat4& viewProjection);

private:
    void quad(int texture, const glm::mat4& model, glm::vec2 size, const glm::vec4& uv,
              const glm::vec4& color);
    void vertex(uint32_t& cursor, const glm::vec3& position, float u, float v, const glm::vec4& color);

    cinder::Gfx::Asset::Assets& assets_;
    const SpritePipeline& pipeline_;

    std::vector<cinder::Gfx::RHI::GpuBuffer> vertexBuffers_;
    std::unique_ptr<cinder::Gfx::RHI::GpuBuffer> indexBuffer_;

    std::vector<float> vertices_;
    std::vector<int> quadTexture_;
    uint32_t quadCount_ = 0;
    bool overflowWarned_ = false;
};

}
