#include "gfx/asset/Mesh.hpp"

#include "gfx/vk/Commands.hpp"

#include "gfx/vk/VkCtx.hpp"

#include <glm/gtc/constants.hpp>

#include <cmath>
#include <cstring>

namespace cinder::gfx::asset {

using cinder::gfx::rhi::GpuBuffer;
using cinder::gfx::vk::VkCtx;

namespace {

void cross(const float* a, const float* b, float* out) {
    out[0] = a[1] * b[2] - a[2] * b[1];
    out[1] = a[2] * b[0] - a[0] * b[2];
    out[2] = a[0] * b[1] - a[1] * b[0];
}

}

Mesh::Mesh(const VkCtx& ctx, const std::vector<float>& vertices,
           const std::vector<uint32_t>& indices)
    : indexCount_(static_cast<uint32_t>(indices.size())) {
    const VkDeviceSize vertexBytes = vertices.size() * sizeof(float);
    const VkDeviceSize indexBytes = indices.size() * sizeof(uint32_t);

    vertexBuffer_ = std::make_unique<GpuBuffer>(ctx, vertexBytes,
                                                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, true);
    indexBuffer_ = std::make_unique<GpuBuffer>(ctx, indexBytes,
                                               VK_BUFFER_USAGE_INDEX_BUFFER_BIT, true);

    std::memcpy(vertexBuffer_->mapped(), vertices.data(), static_cast<std::size_t>(vertexBytes));
    std::memcpy(indexBuffer_->mapped(), indices.data(), static_cast<std::size_t>(indexBytes));
}

Mesh Mesh::cube(const VkCtx& ctx, float r, float g, float b) {
    const float normals[6][3] = {
        {0, 0, 1}, {0, 0, -1},
        {1, 0, 0}, {-1, 0, 0},
        {0, 1, 0}, {0, -1, 0},
    };

    std::vector<float> vertices(24 * 12);
    std::vector<uint32_t> indices(36);
    std::size_t v = 0;
    std::size_t i = 0;

    for (int f = 0; f < 6; ++f) {
        const float* n = normals[f];
        const float guideY[3] = {0, 0, 1};
        const float guideZ[3] = {0, 1, 0};
        const float* guide = std::fabs(n[1]) > 0.5f ? guideY : guideZ;

        float tx[3];
        float ty[3];
        cross(guide, n, tx);
        cross(n, tx, ty);

        for (int c = 0; c < 4; ++c) {
            const float sx = (c == 0 || c == 3) ? -0.5f : 0.5f;
            const float sy = (c < 2) ? -0.5f : 0.5f;

            vertices[v++] = tx[0] * sx + ty[0] * sy + n[0] * 0.5f;
            vertices[v++] = tx[1] * sx + ty[1] * sy + n[1] * 0.5f;
            vertices[v++] = tx[2] * sx + ty[2] * sy + n[2] * 0.5f;
            vertices[v++] = n[0];
            vertices[v++] = n[1];
            vertices[v++] = n[2];
            vertices[v++] = (c == 0 || c == 3) ? 0.0f : 1.0f;
            vertices[v++] = (c < 2) ? 0.0f : 1.0f;
            vertices[v++] = r;
            vertices[v++] = g;
            vertices[v++] = b;
            vertices[v++] = 1.0f;
        }

        const uint32_t base = static_cast<uint32_t>(f) * 4;
        indices[i++] = base;
        indices[i++] = base + 1;
        indices[i++] = base + 2;
        indices[i++] = base + 2;
        indices[i++] = base + 3;
        indices[i++] = base;
    }

    return Mesh(ctx, vertices, indices);
}

Mesh Mesh::sphere(const VkCtx& ctx, float r, float g, float b) {
    constexpr int SEGMENTS = 32;
    constexpr int RINGS = 16;

    std::vector<float> vertices;
    std::vector<uint32_t> indices;
    vertices.reserve(static_cast<std::size_t>((RINGS + 1) * (SEGMENTS + 1) * 12));
    indices.reserve(static_cast<std::size_t>(RINGS * SEGMENTS * 6));

    for (int ring = 0; ring <= RINGS; ++ring) {
        const float polar = glm::pi<float>() * static_cast<float>(ring) / RINGS;
        for (int segment = 0; segment <= SEGMENTS; ++segment) {
            const float u = static_cast<float>(segment) / SEGMENTS;
            const float azimuth = glm::two_pi<float>() * u;
            const float nx = std::sin(polar) * std::cos(azimuth);
            const float ny = std::cos(polar);
            const float nz = std::sin(polar) * std::sin(azimuth);
            const float v = 1.0f - static_cast<float>(ring) / RINGS;
            vertices.insert(vertices.end(), {nx * 0.5f, ny * 0.5f, nz * 0.5f, nx, ny, nz, u, v, r, g, b, 1.0f});
        }
    }

    for (int ring = 0; ring < RINGS; ++ring) {
        for (int segment = 0; segment < SEGMENTS; ++segment) {
            const uint32_t top = static_cast<uint32_t>(ring * (SEGMENTS + 1) + segment);
            const uint32_t bottom = top + SEGMENTS + 1;
            indices.insert(indices.end(), {top, top + 1, bottom, bottom, top + 1, bottom + 1});
        }
    }

    return Mesh(ctx, vertices, indices);
}

Mesh Mesh::capsule(const VkCtx& ctx, float r, float g, float b) {
    constexpr int SEGMENTS = 32;
    constexpr int RINGS = 8;
    constexpr float RADIUS = 0.5f;
    constexpr float HALF = 0.5f;
    constexpr int ROWS = 2 * (RINGS + 1);

    std::vector<float> vertices;
    std::vector<uint32_t> indices;
    vertices.reserve(static_cast<std::size_t>(ROWS * (SEGMENTS + 1) * 12));
    indices.reserve(static_cast<std::size_t>((ROWS - 1) * SEGMENTS * 6));

    int row = 0;
    for (int half = 0; half < 2; ++half) {
        for (int ring = 0; ring <= RINGS; ++ring) {
            const float polar = glm::half_pi<float>() *
                                (static_cast<float>(half) + static_cast<float>(ring) / RINGS);
            const float ny = std::cos(polar);
            const float radial = std::sin(polar);
            const float centre = half == 0 ? HALF : -HALF;
            const float v = 1.0f - static_cast<float>(row) / (ROWS - 1);

            for (int segment = 0; segment <= SEGMENTS; ++segment) {
                const float u = static_cast<float>(segment) / SEGMENTS;
                const float azimuth = glm::two_pi<float>() * u;
                const float nx = radial * std::cos(azimuth);
                const float nz = radial * std::sin(azimuth);
                vertices.insert(vertices.end(), {nx * RADIUS, ny * RADIUS + centre, nz * RADIUS, nx, ny,
                                                 nz, u, v, r, g, b, 1.0f});
            }
            ++row;
        }
    }

    for (int i = 0; i + 1 < ROWS; ++i) {
        for (int segment = 0; segment < SEGMENTS; ++segment) {
            const uint32_t top = static_cast<uint32_t>(i * (SEGMENTS + 1) + segment);
            const uint32_t bottom = top + SEGMENTS + 1;
            indices.insert(indices.end(), {top, top + 1, bottom, bottom, top + 1, bottom + 1});
        }
    }

    return Mesh(ctx, vertices, indices);
}

void Mesh::bind(cinder::gfx::rhi::Commands cmd) const {
    vertexBuffer_->bindVertex(cmd);
    indexBuffer_->bindIndex(cmd);
}

}
