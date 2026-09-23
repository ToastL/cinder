#include "gfx/pass/SpriteBatch.hpp"

#include "gfx/rhi/Commands.hpp"
#include "gfx/vk/Commands.hpp"

#include "gfx/pass/Overflow.hpp"
#include "gfx/vk/VkCtx.hpp"

#include <cstring>

namespace cinder::gfx::pass {

using cinder::gfx::rhi::GpuBuffer;
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

void SpriteBatch::vertex(uint32_t& cursor, const glm::vec3& position, float u, float v,
                         const glm::vec4& color) {
    vertices_[cursor++] = position.x;
    vertices_[cursor++] = position.y;
    vertices_[cursor++] = position.z;
    vertices_[cursor++] = u;
    vertices_[cursor++] = v;
    vertices_[cursor++] = color.r;
    vertices_[cursor++] = color.g;
    vertices_[cursor++] = color.b;
    vertices_[cursor++] = color.a;
}

void SpriteBatch::quad(int texture, const glm::mat4& model, glm::vec2 size, const glm::vec4& uv,
                       const glm::vec4& color) {
    if (quadCount_ >= MAX_QUADS) {
        overflowWarned_ = warnOverflow(overflowWarned_, "sprite batch", "10000 quads");
        return;
    }

    const glm::vec2 half = size * 0.5f;
    const glm::vec3 topLeft(model * glm::vec4(-half.x, half.y, 0.0f, 1.0f));
    const glm::vec3 topRight(model * glm::vec4(half.x, half.y, 0.0f, 1.0f));
    const glm::vec3 bottomRight(model * glm::vec4(half.x, -half.y, 0.0f, 1.0f));
    const glm::vec3 bottomLeft(model * glm::vec4(-half.x, -half.y, 0.0f, 1.0f));

    uint32_t cursor = quadCount_ * FLOATS_PER_QUAD;
    vertex(cursor, topLeft, uv.x, uv.y, color);
    vertex(cursor, topRight, uv.z, uv.y, color);
    vertex(cursor, bottomRight, uv.z, uv.w, color);
    vertex(cursor, bottomLeft, uv.x, uv.w, color);

    quadTexture_[quadCount_] = texture;
    quadCount_++;
}

void SpriteBatch::draw(int texture, const glm::mat4& model, glm::vec2 size, const glm::vec4& color) {
    quad(texture, model, size, glm::vec4(0.0f, 0.0f, 1.0f, 1.0f), color);
}

void SpriteBatch::drawRegion(int texture, const glm::mat4& model, glm::vec2 size,
                             const glm::vec4& region, const glm::vec4& color) {
    const cinder::gfx::rhi::Texture& tex = assets_.get(texture);
    const glm::vec2 texels(static_cast<float>(tex.width()), static_cast<float>(tex.height()));
    const glm::vec2 from = glm::vec2(region.x, region.y) / texels;
    const glm::vec2 to = glm::vec2(region.x + region.z, region.y + region.w) / texels;
    quad(texture, model, size, glm::vec4(from, to), color);
}

void SpriteBatch::flush(cinder::gfx::rhi::Commands cmd, uint32_t frameIndex,
                        const glm::mat4& viewProjection) {
    if (quadCount_ == 0) return;

    std::memcpy(vertexBuffers_[frameIndex].mapped(), vertices_.data(),
                static_cast<std::size_t>(quadCount_) * FLOATS_PER_QUAD * sizeof(float));

    pipeline_.bind(cmd, viewProjection);

    vertexBuffers_[frameIndex].bindVertex(cmd);
    indexBuffer_->bindIndex(cmd);

    uint32_t start = 0;
    while (start < quadCount_) {
        const int texture = quadTexture_[start];
        uint32_t end = start;
        while (end < quadCount_ && quadTexture_[end] == texture) end++;

        pipeline_.bindTexture(cmd, assets_.get(texture).binding());
        cinder::gfx::rhi::drawIndexed(cmd, (end - start) * 6, start * 6);
        start = end;
    }
}

}
