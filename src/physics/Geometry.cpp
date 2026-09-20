#include "physics/Geometry.hpp"

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/matrix.hpp>

#include <cfloat>

#include <algorithm>
#include <cmath>

namespace cinder::physics {

glm::vec3 scaleOf(const glm::mat4& world) {
    return glm::vec3(glm::length(glm::vec3(world[0])), glm::length(glm::vec3(world[1])),
                     glm::length(glm::vec3(world[2])));
}

glm::mat3 axesOf(const glm::mat4& world) {
    glm::mat3 axes(1.0f);
    for (int i = 0; i < 3; ++i) {
        const glm::vec3 column(world[i]);
        const float length = glm::length(column);
        if (length > 0.0f) axes[i] = column / length;
    }
    return axes;
}

Geometry geometryOf(Shape shape, const glm::vec3& size, const glm::mat4& world) {
    Geometry out;
    out.shape = shape;
    out.center = glm::vec3(world[3]);
    out.axes = axesOf(world);

    const glm::vec3 extents = size * scaleOf(world) * 0.5f;
    if (shape == Shape::Sphere) {
        out.radius = std::max({extents.x, extents.y, extents.z});
        out.halfExtents = glm::vec3(out.radius);
    } else {
        out.halfExtents = extents;
    }
    return out;
}

Bounds boundsOf(const Geometry& geometry) {
    glm::vec3 reach(geometry.radius);
    if (geometry.shape == Shape::Box) {
        const glm::mat3 absolute(glm::abs(geometry.axes[0]), glm::abs(geometry.axes[1]),
                                 glm::abs(geometry.axes[2]));
        reach = absolute * geometry.halfExtents;
    }
    return {geometry.center - reach, geometry.center + reach};
}

bool overlaps(const Bounds& a, const Bounds& b) {
    return a.min.x <= b.max.x && b.min.x <= a.max.x && a.min.y <= b.max.y && b.min.y <= a.max.y &&
           a.min.z <= b.max.z && b.min.z <= a.max.z;
}

float volumeOf(const Geometry& geometry) {
    if (geometry.shape == Shape::Sphere) {
        return 4.0f / 3.0f * glm::pi<float>() * geometry.radius * geometry.radius * geometry.radius;
    }
    const glm::vec3& h = geometry.halfExtents;
    return 8.0f * h.x * h.y * h.z;
}

namespace {

bool raySphere(const Geometry& sphere, const glm::vec3& origin, const glm::vec3& direction,
               float& distance, glm::vec3& normal) {
    const glm::vec3 offset = origin - sphere.center;
    const float along = glm::dot(offset, direction);
    const float gap = glm::dot(offset, offset) - sphere.radius * sphere.radius;
    const float discriminant = along * along - gap;
    if (discriminant < 0.0f) return false;

    const float entry = -along - std::sqrt(discriminant);
    if (entry < 0.0f) return false;

    distance = entry;
    normal = (origin + direction * entry - sphere.center) / sphere.radius;
    return true;
}

bool rayBox(const Geometry& box, const glm::vec3& origin, const glm::vec3& direction,
            float& distance, glm::vec3& normal) {
    const glm::mat3 into = glm::transpose(box.axes);
    const glm::vec3 start = into * (origin - box.center);
    const glm::vec3 heading = into * direction;

    float entry = 0.0f;
    float exit = FLT_MAX;
    int axis = 0;
    float side = 1.0f;

    for (int i = 0; i < 3; ++i) {
        if (std::abs(heading[i]) < 1e-8f) {
            if (std::abs(start[i]) > box.halfExtents[i]) return false;
            continue;
        }

        const float inverse = 1.0f / heading[i];
        float enters = (-box.halfExtents[i] - start[i]) * inverse;
        float leaves = (box.halfExtents[i] - start[i]) * inverse;
        float facing = -1.0f;
        if (enters > leaves) {
            std::swap(enters, leaves);
            facing = 1.0f;
        }
        if (enters > entry) {
            entry = enters;
            axis = i;
            side = facing;
        }
        exit = std::min(exit, leaves);
        if (entry > exit) return false;
    }

    if (entry <= 0.0f) return false;

    distance = entry;
    normal = box.axes[axis] * side;
    return true;
}

}

bool rayHits(const Geometry& geometry, const glm::vec3& origin, const glm::vec3& direction,
             float& distance, glm::vec3& normal) {
    return geometry.shape == Shape::Sphere ? raySphere(geometry, origin, direction, distance, normal)
                                           : rayBox(geometry, origin, direction, distance, normal);
}

glm::mat3 inertiaOf(const Geometry& geometry, float mass) {
    if (geometry.shape == Shape::Sphere) {
        return glm::mat3(0.4f * mass * geometry.radius * geometry.radius);
    }
    const glm::vec3 h2 = geometry.halfExtents * geometry.halfExtents;
    glm::mat3 local(0.0f);
    local[0][0] = mass / 3.0f * (h2.y + h2.z);
    local[1][1] = mass / 3.0f * (h2.x + h2.z);
    local[2][2] = mass / 3.0f * (h2.x + h2.y);
    return geometry.axes * local * glm::transpose(geometry.axes);
}

}
