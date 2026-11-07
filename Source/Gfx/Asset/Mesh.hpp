#pragma once

#include "Gfx/RHI/Handles.hpp"
#include "Gfx/RHI/Backend.hpp"

#include <memory>
#include <vector>

namespace cinder::Gfx::Asset {

class Mesh {
public:
    static constexpr uint32_t VERTEX_FLOATS = 12;
    static constexpr uint32_t VERTEX_STRIDE = VERTEX_FLOATS * sizeof(float);
    static constexpr uint32_t NORMAL_OFFSET = 3 * sizeof(float);
    static constexpr uint32_t UV_OFFSET = 6 * sizeof(float);
    static constexpr uint32_t COLOR_OFFSET = 8 * sizeof(float);

    Mesh(const cinder::Gfx::RHI::Ctx& ctx, const std::vector<float>& vertices,
         const std::vector<uint32_t>& indices);

    static Mesh cube(const cinder::Gfx::RHI::Ctx& ctx, float r, float g, float b);
    static Mesh sphere(const cinder::Gfx::RHI::Ctx& ctx, float r, float g, float b);
    static Mesh capsule(const cinder::Gfx::RHI::Ctx& ctx, float r, float g, float b);

    void bind(cinder::Gfx::RHI::Commands cmd) const;
    uint32_t indexCount() const { return indexCount_; }

private:
    std::unique_ptr<cinder::Gfx::RHI::GpuBuffer> vertexBuffer_;
    std::unique_ptr<cinder::Gfx::RHI::GpuBuffer> indexBuffer_;
    uint32_t indexCount_ = 0;
};

}
