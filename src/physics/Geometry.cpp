#include "physics/Geometry.hpp"

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>

#include <algorithm>

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
