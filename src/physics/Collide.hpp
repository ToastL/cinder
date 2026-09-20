#pragma once

#include "physics/Geometry.hpp"

#include <glm/vec3.hpp>

#include <array>

namespace cinder::physics {

struct ContactPoint {
    glm::vec3 position{0.0f};
    float depth = 0.0f;
};

struct Manifold {
    static constexpr int MAX_POINTS = 4;

    glm::vec3 normal{0.0f, 1.0f, 0.0f};
    std::array<ContactPoint, MAX_POINTS> points{};
    int count = 0;
};

bool collide(const Geometry& a, const Geometry& b, Manifold& out);

}
