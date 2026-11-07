#include "Physics/World.hpp"

#include "Physics/Body.hpp"
#include "Physics/Collider.hpp"
#include "Physics/Geometry.hpp"
#include "Scene/Node.hpp"
#include "Scene/Scene.hpp"

#include <glm/geometric.hpp>

namespace cinder::Physics {

using cinder::Scene::Node;

bool World::raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance,
                    RayHit& hit) const {
    const float reach = glm::length(direction);
    if (reach <= 0.0f || maxDistance <= 0.0f) return false;

    const glm::vec3 heading = direction / reach;
    bool found = false;
    for (Node* root : scene_.roots()) cast(*root, nullptr, origin, heading, maxDistance, hit, found);
    return found;
}

void World::cast(Node& node, Body* owner, const glm::vec3& origin, const glm::vec3& direction,
                 float maxDistance, RayHit& hit, bool& found) const {
    if (node.destroyed() || !node.isEnabled()) return;

    if (auto* body = dynamic_cast<Body*>(&node)) {
        owner = body;
    } else if (auto* collider = dynamic_cast<Collider*>(&node)) {
        float distance = 0.0f;
        glm::vec3 normal(0.0f);
        const Geometry geometry = collider->geometry();
        if (rayHits(geometry, origin, direction, distance, normal) && distance <= maxDistance &&
            (!found || distance < hit.distance)) {
            hit.node = owner != nullptr ? static_cast<Node*>(owner) : static_cast<Node*>(collider);
            hit.collider = collider;
            hit.position = origin + direction * distance;
            hit.normal = normal;
            hit.distance = distance;
            found = true;
        }
    }

    for (Node* child : node.children()) cast(*child, owner, origin, direction, maxDistance, hit, found);
}

}
