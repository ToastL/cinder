#pragma once

#include "reflect/Enum.hpp"

#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace cinder::physics {

enum class Shape { Box, Sphere };

struct Geometry {
    Shape shape = Shape::Box;
    glm::vec3 center{0.0f};
    glm::mat3 axes{1.0f};
    glm::vec3 halfExtents{0.0f};
    float radius = 0.0f;
};

struct Bounds {
    glm::vec3 min{0.0f};
    glm::vec3 max{0.0f};
};

glm::vec3 scaleOf(const glm::mat4& world);
glm::mat3 axesOf(const glm::mat4& world);

Geometry geometryOf(Shape shape, const glm::vec3& size, const glm::mat4& world);
Bounds boundsOf(const Geometry& geometry);
bool overlaps(const Bounds& a, const Bounds& b);
float volumeOf(const Geometry& geometry);
glm::mat3 inertiaOf(const Geometry& geometry, float mass);
bool rayHits(const Geometry& geometry, const glm::vec3& origin, const glm::vec3& direction,
             float& distance, glm::vec3& normal);

}

CINDER_ENUM_NAMES(cinder::physics::Shape, "box", "sphere")
