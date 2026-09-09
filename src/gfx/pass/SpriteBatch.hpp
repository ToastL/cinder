#pragma once

#include "gfx/asset/Assets.hpp"
#include "gfx/pass/SpritePipeline.hpp"
#include "gfx/vk/GpuBuffer.hpp"

#include <memory>
#include <vector>

namespace cinder::gfx::pass {

class SpriteBatch {
public:
    static constexpr uint32_t MAX_QUADS = 10000;
    static constexpr uint32_t FLOATS_PER_QUAD = 4 * SpritePipeline::FLOATS_PER_VERTEX;

    SpriteBatch(const cinder::gfx::vk::VkCtx& ctx, cinder::gfx::asset::Assets& assets,
                const SpritePipeline& pipeline, uint32_t framesInFlight);

    void reset() { quadCount_ = 0; }

    void draw(int texture, float x, float y, float w, float h, float rot,
              float r, float g, float b, float a);
    void drawRegion(int texture, float x, float y, float w, float h,
                    float sx, float sy, float sw, float sh, float rot,
                    float r, float g, float b, float a);

    void flush(VkCommandBuffer cmd, uint32_t frameIndex, const glm::mat4& projection);

private:
    void vertex(uint32_t& cursor, float cx, float cy, float cos, float sin,
                float ox, float oy, float u, float v,
                float r, float g, float b, float a);

    cinder::gfx::asset::Assets& assets_;
    const SpritePipeline& pipeline_;

    std::vector<cinder::gfx::vk::GpuBuffer> vertexBuffers_;
    std::unique_ptr<cinder::gfx::vk::GpuBuffer> indexBuffer_;

    std::vector<float> vertices_;
    std::vector<int> quadTexture_;
    uint32_t quadCount_ = 0;
    bool overflowWarned_ = false;
};

}
