#pragma once

#include "gfx/rhi/Handles.hpp"
#include "gfx/vk/GpuBuffer.hpp"

#include <memory>
#include <vector>

namespace cinder::gfx::asset {

class Mesh {
public:
    static constexpr uint32_t VERTEX_STRIDE = 12 * sizeof(float);

    Mesh(const cinder::gfx::vk::VkCtx& ctx, const std::vector<float>& vertices,
         const std::vector<uint32_t>& indices);

    static Mesh cube(const cinder::gfx::vk::VkCtx& ctx, float r, float g, float b);
    static Mesh sphere(const cinder::gfx::vk::VkCtx& ctx, float r, float g, float b);
    static Mesh capsule(const cinder::gfx::vk::VkCtx& ctx, float r, float g, float b);

    void bind(cinder::gfx::rhi::Commands cmd) const;
    uint32_t indexCount() const { return indexCount_; }

private:
    std::unique_ptr<cinder::gfx::vk::GpuBuffer> vertexBuffer_;
    std::unique_ptr<cinder::gfx::vk::GpuBuffer> indexBuffer_;
    uint32_t indexCount_ = 0;
};

}
