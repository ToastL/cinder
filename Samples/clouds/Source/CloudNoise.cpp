#include "CloudNoise.hpp"

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <thread>

namespace {

std::uint32_t hash(std::int32_t x, std::int32_t y, std::int32_t z, std::uint32_t seed) {
    std::uint32_t h = seed * 0x9e3779b9u;
    h ^= static_cast<std::uint32_t>(x) * 0x85ebca6bu;
    h = (h ^ (h >> 13)) * 0xc2b2ae35u;
    h ^= static_cast<std::uint32_t>(y) * 0x27d4eb2fu;
    h = (h ^ (h >> 15)) * 0x165667b1u;
    h ^= static_cast<std::uint32_t>(z) * 0x9e3779b1u;
    h = (h ^ (h >> 16)) * 0x85ebca6bu;
    return h ^ (h >> 13);
}

float unit(std::uint32_t h) { return static_cast<float>(h & 0xffffffu) / static_cast<float>(0x1000000); }

std::int32_t wrap(std::int32_t value, std::int32_t period) { return ((value % period) + period) % period; }

float worley(glm::vec3 p, std::int32_t cells, std::uint32_t seed) {
    const glm::vec3 scaled = p * static_cast<float>(cells);
    const glm::vec3 cell = glm::floor(scaled);
    float nearest = 1.0f;
    for (std::int32_t dz = -1; dz <= 1; ++dz) {
        for (std::int32_t dy = -1; dy <= 1; ++dy) {
            for (std::int32_t dx = -1; dx <= 1; ++dx) {
                const std::int32_t cx = static_cast<std::int32_t>(cell.x) + dx;
                const std::int32_t cy = static_cast<std::int32_t>(cell.y) + dy;
                const std::int32_t cz = static_cast<std::int32_t>(cell.z) + dz;
                const std::uint32_t h = hash(wrap(cx, cells), wrap(cy, cells), wrap(cz, cells), seed);
                const glm::vec3 feature(static_cast<float>(cx) + unit(h), static_cast<float>(cy) + unit(h * 747796405u + 1u),
                                        static_cast<float>(cz) + unit(h * 2891336453u + 7u));
                nearest = std::min(nearest, glm::length(feature - scaled));
            }
        }
    }
    return 1.0f - std::clamp(nearest, 0.0f, 1.0f);
}

float worleyFbm(glm::vec3 p, std::int32_t cells, std::uint32_t seed) {
    return worley(p, cells, seed) * 0.625f + worley(p, cells * 2, seed + 1) * 0.25f
            + worley(p, cells * 4, seed + 2) * 0.125f;
}

glm::vec3 gradient(std::int32_t x, std::int32_t y, std::int32_t z, std::uint32_t seed) {
    const std::uint32_t h = hash(x, y, z, seed);
    const float theta = unit(h) * 6.2831853f;
    const float cosPhi = unit(h * 747796405u + 3u) * 2.0f - 1.0f;
    const float sinPhi = std::sqrt(1.0f - cosPhi * cosPhi);
    return {sinPhi * std::cos(theta), sinPhi * std::sin(theta), cosPhi};
}

float fade(float t) { return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f); }

float perlin(glm::vec3 p, std::int32_t cells, std::uint32_t seed) {
    const glm::vec3 scaled = p * static_cast<float>(cells);
    const glm::vec3 cell = glm::floor(scaled);
    const glm::vec3 f = scaled - cell;
    const std::int32_t x0 = static_cast<std::int32_t>(cell.x);
    const std::int32_t y0 = static_cast<std::int32_t>(cell.y);
    const std::int32_t z0 = static_cast<std::int32_t>(cell.z);
    float corners[8];
    for (std::int32_t i = 0; i < 8; ++i) {
        const std::int32_t ox = i & 1;
        const std::int32_t oy = (i >> 1) & 1;
        const std::int32_t oz = (i >> 2) & 1;
        const glm::vec3 g = gradient(wrap(x0 + ox, cells), wrap(y0 + oy, cells), wrap(z0 + oz, cells), seed);
        corners[i] = glm::dot(g, f - glm::vec3(ox, oy, oz));
    }
    const float u = fade(f.x);
    const float v = fade(f.y);
    const float w = fade(f.z);
    const float x00 = glm::mix(corners[0], corners[1], u);
    const float x10 = glm::mix(corners[2], corners[3], u);
    const float x01 = glm::mix(corners[4], corners[5], u);
    const float x11 = glm::mix(corners[6], corners[7], u);
    return glm::mix(glm::mix(x00, x10, v), glm::mix(x01, x11, v), w) * 0.5f + 0.5f;
}

float perlinFbm(glm::vec3 p, std::int32_t cells, std::uint32_t seed, std::int32_t octaves) {
    float sum = 0.0f;
    float weight = 0.0f;
    float amplitude = 1.0f;
    for (std::int32_t i = 0; i < octaves; ++i) {
        sum += perlin(p, cells << i, seed + static_cast<std::uint32_t>(i)) * amplitude;
        weight += amplitude;
        amplitude *= 0.5f;
    }
    return sum / weight;
}

float remap(float value, float low, float high, float newLow, float newHigh) {
    return newLow + (value - low) * (newHigh - newLow) / (high - low);
}

std::uint8_t toByte(float value) {
    return static_cast<std::uint8_t>(std::lround(std::clamp(value, 0.0f, 1.0f) * 255.0f));
}

void fill(std::vector<std::uint8_t>& out, std::uint32_t size, std::uint32_t depth,
          const std::function<void(glm::vec3, std::uint8_t*)>& voxel) {
    out.resize(static_cast<std::size_t>(size) * size * depth * 4);
    const std::uint32_t workers = std::max(1u, std::thread::hardware_concurrency());
    std::vector<std::thread> threads;
    for (std::uint32_t worker = 0; worker < workers; ++worker) {
        threads.emplace_back([&, worker] {
            for (std::uint32_t z = worker; z < depth; z += workers) {
                for (std::uint32_t y = 0; y < size; ++y) {
                    for (std::uint32_t x = 0; x < size; ++x) {
                        const glm::vec3 p((static_cast<float>(x) + 0.5f) / static_cast<float>(size),
                                          (static_cast<float>(y) + 0.5f) / static_cast<float>(size),
                                          (static_cast<float>(z) + 0.5f) / static_cast<float>(depth));
                        voxel(p, out.data() + ((static_cast<std::size_t>(z) * size + y) * size + x) * 4);
                    }
                }
            }
        });
    }
    for (std::thread& thread : threads) thread.join();
}

CloudNoise generate() {
    CloudNoise noise;
    fill(noise.shape, CloudNoise::SHAPE_SIZE, CloudNoise::SHAPE_SIZE, [](glm::vec3 p, std::uint8_t* texel) {
        const float worleyLow = worleyFbm(p, 4, 11);
        const float perlinWorley = remap(perlinFbm(p, 4, 3, 3), 0.0f, 1.0f, worleyLow, 1.0f);
        texel[0] = toByte(perlinWorley);
        texel[1] = toByte(worleyLow);
        texel[2] = toByte(worleyFbm(p, 8, 23));
        texel[3] = toByte(worleyFbm(p, 16, 31));
    });
    fill(noise.detail, CloudNoise::DETAIL_SIZE, CloudNoise::DETAIL_SIZE, [](glm::vec3 p, std::uint8_t* texel) {
        texel[0] = toByte(worleyFbm(p, 2, 41));
        texel[1] = toByte(worleyFbm(p, 4, 53));
        texel[2] = toByte(worleyFbm(p, 8, 61));
        texel[3] = 255;
    });
    fill(noise.weather, CloudNoise::WEATHER_SIZE, 1, [](glm::vec3 p, std::uint8_t* texel) {
        const glm::vec3 flat(p.x, p.y, 0.0f);
        const float blobs = worley(flat, 4, 71) * 0.75f + worley(flat, 8, 72) * 0.25f;
        const float clusters = std::clamp(remap(perlinFbm(flat, 2, 73, 2), 0.35f, 0.6f, 0.5f, 1.0f), 0.5f, 1.0f);
        texel[0] = toByte(remap(blobs, 0.15f, 0.75f, 0.0f, 1.0f) * clusters);
        texel[1] = toByte(remap(perlinFbm(flat, 2, 83, 2), 0.35f, 0.65f, 0.0f, 1.0f));
        texel[2] = toByte(remap(perlinFbm(flat, 3, 97, 2), 0.3f, 0.7f, 0.0f, 1.0f));
        texel[3] = 255;
    });
    return noise;
}

}

const CloudNoise& cloudNoise() {
    static const CloudNoise noise = generate();
    return noise;
}
