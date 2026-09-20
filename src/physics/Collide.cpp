#include "physics/Collide.hpp"

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/matrix.hpp>

#include <cmath>

namespace cinder::physics {

namespace {

constexpr float COINCIDENT = 1e-6f;
const glm::vec3 UP(0.0f, 1.0f, 0.0f);

bool sphereSphere(const Geometry& a, const Geometry& b, Manifold& out) {
    const glm::vec3 delta = b.center - a.center;
    const float reach = a.radius + b.radius;
    const float distanceSq = glm::dot(delta, delta);
    if (distanceSq > reach * reach) return false;

    const float distance = std::sqrt(distanceSq);
    const float depth = reach - distance;
    out.normal = distance > COINCIDENT ? delta / distance : UP;
    out.points[0] = {a.center + out.normal * (a.radius - depth * 0.5f), depth};
    out.count = 1;
    return true;
}

bool sphereBox(const Geometry& sphere, const Geometry& box, Manifold& out) {
    const glm::vec3 local = glm::transpose(box.axes) * (sphere.center - box.center);
    const glm::vec3 closest = glm::clamp(local, -box.halfExtents, box.halfExtents);
    const glm::vec3 offset = local - closest;
    const float distanceSq = glm::dot(offset, offset);

    if (distanceSq > COINCIDENT * COINCIDENT) {
        if (distanceSq > sphere.radius * sphere.radius) return false;
        const float distance = std::sqrt(distanceSq);
        out.normal = -(box.axes * (offset / distance));
        out.points[0] = {box.center + box.axes * closest, sphere.radius - distance};
        out.count = 1;
        return true;
    }

    int axis = 0;
    float gap = box.halfExtents[0] - std::abs(local[0]);
    for (int i = 1; i < 3; ++i) {
        const float candidate = box.halfExtents[i] - std::abs(local[i]);
        if (candidate < gap) {
            gap = candidate;
            axis = i;
        }
    }

    const float side = local[axis] < 0.0f ? -1.0f : 1.0f;
    glm::vec3 face = local;
    face[axis] = side * box.halfExtents[axis];
    out.normal = -box.axes[axis] * side;
    out.points[0] = {box.center + box.axes * face, sphere.radius + gap};
    out.count = 1;
    return true;
}

}

bool collide(const Geometry& a, const Geometry& b, Manifold& out) {
    out.count = 0;
    if (a.shape == Shape::Sphere && b.shape == Shape::Sphere) return sphereSphere(a, b, out);
    if (a.shape == Shape::Sphere && b.shape == Shape::Box) return sphereBox(a, b, out);
    if (a.shape == Shape::Box && b.shape == Shape::Sphere) {
        if (!sphereBox(b, a, out)) return false;
        out.normal = -out.normal;
        return true;
    }
    return false;
}

}
