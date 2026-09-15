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
    Node* sprite = nullptr;
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
        if (hitSprite(view.world2d, sprite->transform()->world(), sprite->size())) hits.sprite = &node;
    } else if (auto* camera = dynamic_cast<Camera*>(&node)) {
        if (camera->projection() == Camera::Projection::Perspective) {
            distance = hitMarker(view, ray, camera->transform()->worldPosition());
        }
    }

    if (distance && *distance < hits.distance) {
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

SpriteRect spriteRect(const glm::mat4& world, glm::vec2 size) {
    const glm::vec2 scaled(size.x * glm::length(glm::vec3(world[0])), size.y * glm::length(glm::vec3(world[1])));
    return SpriteRect{glm::vec2(world[3]), scaled * 0.5f, std::atan2(world[0][1], world[0][0])};
}

bool hitSprite(glm::vec2 point, const glm::mat4& world, glm::vec2 size) {
    const SpriteRect rect = spriteRect(world, size);
    const glm::vec2 offset = point - rect.centre;
    const float cos = std::cos(rect.rotation);
    const float sin = std::sin(rect.rotation);
    const glm::vec2 local(offset.x * cos + offset.y * sin, -offset.x * sin + offset.y * cos);
    return std::abs(local.x) <= rect.half.x && std::abs(local.y) <= rect.half.y;
}

Node* pick(cinder::scene::Scene& scene, const PickView& view) {
    const Ray ray = rayThrough(view.viewProjection, view.point, view.size);
    Hits hits;
    for (Node* root : scene.roots()) visit(*root, view, ray, hits);
    return hits.sprite != nullptr ? hits.sprite : hits.nearest;
}

}
