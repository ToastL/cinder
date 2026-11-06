#include "dev/Picking.hpp"

#include "components/Camera.hpp"
#include "components/MeshPart.hpp"
#include "components/Sprite.hpp"
#include "scene/Node.hpp"
#include "scene/Scene.hpp"
#include "scene/Transform.hpp"

#include <glm/geometric.hpp>
#include <glm/matrix.hpp>
#include <glm/vec4.hpp>

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <utility>

namespace cinder::dev {
namespace {

using cinder::components::Camera;
using cinder::components::MeshPart;
using cinder::components::Sprite;
using cinder::scene::Node;

constexpr float HALF_EXTENT = 0.5f;
constexpr float MARKER_RADIUS = 10.0f;
constexpr float DEGENERATE = 1e-12f;
constexpr float PARALLEL = 1e-8f;

struct Hits {
    Node* nearest = nullptr;
    float distance = FLT_MAX;
};

std::optional<float> hitMarker(const PickView& view, const Ray& ray, const glm::vec3& position) {
    const glm::vec4 clip = view.viewProjection * glm::vec4(position, 1.0f);
    if (clip.w <= PARALLEL || clip.z < 0.0f || clip.z > clip.w) return std::nullopt;

    const glm::vec2 ndc = glm::vec2(clip) / clip.w;
    const glm::vec2 screen = (ndc * 0.5f + 0.5f) * view.size;
    if (glm::distance(screen, view.point) > MARKER_RADIUS) return std::nullopt;
    return glm::dot(position - ray.origin, ray.direction);
}

void visit(Node& node, const PickView& view, const Ray& ray, Hits& hits) {
    if (node.destroyed() || !node.isEnabled()) return;

    std::optional<float> distance;
    if (auto* part = dynamic_cast<MeshPart*>(&node)) {
        distance = hitCube(ray, part->transform()->world());
    } else if (auto* sprite = dynamic_cast<Sprite*>(&node)) {
        distance = hitSprite(ray, sprite->transform()->world(), sprite->size());
    } else if (auto* camera = dynamic_cast<Camera*>(&node)) {
        distance = hitMarker(view, ray, camera->transform()->worldPosition());
    }

    if (distance && *distance <= hits.distance) {
        hits.distance = *distance;
        hits.nearest = &node;
    }

    for (Node* child : node.children()) visit(*child, view, ray, hits);
}

}

Ray rayThrough(const glm::mat4& viewProjection, glm::vec2 point, glm::vec2 size) {
    const glm::vec2 ndc = point / size * 2.0f - 1.0f;
    const glm::mat4 inverse = glm::inverse(viewProjection);

    glm::vec4 front = inverse * glm::vec4(ndc, 0.0f, 1.0f);
    glm::vec4 back = inverse * glm::vec4(ndc, 1.0f, 1.0f);
    front /= front.w;
    back /= back.w;

    return Ray{glm::vec3(front), glm::normalize(glm::vec3(back - front))};
}

std::optional<float> hitCube(const Ray& ray, const glm::mat4& world) {
    if (std::abs(glm::determinant(world)) < DEGENERATE) return std::nullopt;

    const glm::mat4 inverse = glm::inverse(world);
    const glm::vec3 origin(inverse * glm::vec4(ray.origin, 1.0f));
    const glm::vec3 direction(inverse * glm::vec4(ray.direction, 0.0f));

    float enter = -FLT_MAX;
    float exit = FLT_MAX;
    for (int axis = 0; axis < 3; ++axis) {
        if (std::abs(direction[axis]) < PARALLEL) {
            if (origin[axis] < -HALF_EXTENT || origin[axis] > HALF_EXTENT) return std::nullopt;
            continue;
        }

        float a = (-HALF_EXTENT - origin[axis]) / direction[axis];
        float b = (HALF_EXTENT - origin[axis]) / direction[axis];
        if (a > b) std::swap(a, b);
        enter = std::max(enter, a);
        exit = std::min(exit, b);
        if (enter > exit) return std::nullopt;
    }

    if (exit < 0.0f) return std::nullopt;
    return std::max(enter, 0.0f);
}

std::optional<float> hitSprite(const Ray& ray, const glm::mat4& world, glm::vec2 size) {
    const glm::vec3 across(world[0]);
    const glm::vec3 up(world[1]);
    const glm::vec3 normal = glm::cross(across, up);
    const float area = glm::dot(normal, normal);
    if (area < DEGENERATE) return std::nullopt;

    const glm::vec3 unit = normal / std::sqrt(area);
    const float facing = glm::dot(ray.direction, unit);
    if (std::abs(facing) < PARALLEL) return std::nullopt;

    const glm::vec3 centre(world[3]);
    const float distance = glm::dot(centre - ray.origin, unit) / facing;
    if (distance < 0.0f) return std::nullopt;

    const glm::vec3 offset = ray.origin + ray.direction * distance - centre;
    const float shear = glm::dot(across, up);
    const float alongAcross = glm::dot(offset, across);
    const float alongUp = glm::dot(offset, up);
    const glm::vec2 local((alongAcross * glm::dot(up, up) - alongUp * shear) / area,
                          (alongUp * glm::dot(across, across) - alongAcross * shear) / area);

    const glm::vec2 half = size * 0.5f;
    if (std::abs(local.x) > half.x || std::abs(local.y) > half.y) return std::nullopt;
    return distance;
}

Node* pick(cinder::scene::Scene& scene, const PickView& view) {
    const Ray ray = rayThrough(view.viewProjection, view.point, view.size);
    Hits hits;
    for (Node* root : scene.roots()) visit(*root, view, ray, hits);
    return hits.nearest;
}

}
