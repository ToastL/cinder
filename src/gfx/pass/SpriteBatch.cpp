#include "gfx/pass/SpriteBatch.hpp"

#include "gfx/pass/Overflow.hpp"
#include "gfx/vk/VkCtx.hpp"

#include <cmath>
#include <cstring>

namespace cinder::gfx::pass {

using cinder::gfx::vk::GpuBuffer;
using cinder::gfx::vk::VkCtx;

SpriteBatch::SpriteBatch(const VkCtx& ctx, cinder::gfx::asset::Assets& assets,
                         const SpritePipeline& pipeline, uint32_t framesInFlight)
    : assets_(assets), pipeline_(pipeline),
      vertices_(static_cast<std::size_t>(MAX_QUADS) * FLOATS_PER_QUAD),
      quadTexture_(MAX_QUADS) {
    const VkDeviceSize vertexBytes =
            static_cast<VkDeviceSize>(MAX_QUADS) * FLOATS_PER_QUAD * sizeof(float);

    vertexBuffers_.reserve(framesInFlight);
    for (uint32_t i = 0; i < framesInFlight; ++i) {
        vertexBuffers_.emplace_back(ctx, vertexBytes, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, true);
    }

    const VkDeviceSize indexBytes = static_cast<VkDeviceSize>(MAX_QUADS) * 6 * sizeof(uint32_t);
    indexBuffer_ = std::make_unique<GpuBuffer>(ctx, indexBytes, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                               true);

    auto* indices = static_cast<uint32_t*>(indexBuffer_->mapped());
    for (uint32_t quad = 0; quad < MAX_QUADS; ++quad) {
        const uint32_t v = quad * 4;
        indices[quad * 6 + 0] = v;
        indices[quad * 6 + 1] = v + 1;
        indices[quad * 6 + 2] = v + 2;
        indices[quad * 6 + 3] = v + 2;
        indices[quad * 6 + 4] = v + 3;
        indices[quad * 6 + 5] = v;
    }
}

void SpriteBatch::vertex(uint32_t& cursor, float cx, float cy, float cos, float sin,
                         float ox, float oy, float u, float v,
                         float r, float g, float b, float a) {
    vertices_[cursor++] = cx + ox * cos - oy * sin;
    vertices_[cursor++] = cy + ox * sin + oy * cos;
    vertices_[cursor++] = u;
    vertices_[cursor++] = v;
    vertices_[cursor++] = r;
    vertices_[cursor++] = g;
    vertices_[cursor++] = b;
    vertices_[cursor++] = a;
}

void SpriteBatch::drawRegion(int texture, float x, float y, float w, float h,
                             float sx, float sy, float sw, float sh, float rot,
                             float r, float g, float b, float a) {
    if (quadCount_ >= MAX_QUADS) {
        overflowWarned_ = warnOverflow(overflowWarned_, "sprite batch", "10000 quads");
        return;
    }

    const cinder::gfx::asset::Texture& tex = assets_.get(texture);
    const float u0 = sx / static_cast<float>(tex.width());
    const float v0 = sy / static_cast<float>(tex.height());
    const float u1 = (sx + sw) / static_cast<float>(tex.width());
    const float v1 = (sy + sh) / static_cast<float>(tex.height());

    const float hw = w * 0.5f;
    const float hh = h * 0.5f;
    const float cx = x + hw;
    const float cy = y + hh;
    const float cos = std::cos(rot);
    const float sin = std::sin(rot);

    uint32_t cursor = quadCount_ * FLOATS_PER_QUAD;
    vertex(cursor, cx, cy, cos, sin, -hw, -hh, u0, v0, r, g, b, a);
    vertex(cursor, cx, cy, cos, sin, hw, -hh, u1, v0, r, g, b, a);
    vertex(cursor, cx, cy, cos, sin, hw, hh, u1, v1, r, g, b, a);
    vertex(cursor, cx, cy, cos, sin, -hw, hh, u0, v1, r, g, b, a);

    quadTexture_[quadCount_] = texture;
    quadCount_++;
}

void SpriteBatch::draw(int texture, float x, float y, float w, float h, float rot,
                       float r, float g, float b, float a) {
    const cinder::gfx::asset::Texture& tex = assets_.get(texture);
    drawRegion(texture, x, y, w, h, 0, 0,
               static_cast<float>(tex.width()), static_cast<float>(tex.height()),
               rot, r, g, b, a);
}

void SpriteBatch::flush(VkCommandBuffer cmd, uint32_t frameIndex, const glm::mat4& projection) {
    if (quadCount_ == 0) return;

    std::memcpy(vertexBuffers_[frameIndex].mapped(), vertices_.data(),
                static_cast<std::size_t>(quadCount_) * FLOATS_PER_QUAD * sizeof(float));

    pipeline_.bind(cmd, projection);

    const VkBuffer vertexBuffer = vertexBuffers_[frameIndex].handle();
    const VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &vertexBuffer, &offset);
    vkCmdBindIndexBuffer(cmd, indexBuffer_->handle(), 0, VK_INDEX_TYPE_UINT32);

    uint32_t start = 0;
    while (start < quadCount_) {
        const int texture = quadTexture_[start];
        uint32_t end = start;
        while (end < quadCount_ && quadTexture_[end] == texture) end++;

        const VkDescriptorSet set = assets_.get(texture).descriptorSet();
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_.layout(),
                                0, 1, &set, 0, nullptr);
        vkCmdDrawIndexed(cmd, (end - start) * 6, 1, start * 6, 0, 0);
        start = end;
    }
}

}
