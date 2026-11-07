#include "Physics/World.hpp"

#include "Physics/RigidBodyComponent.hpp"
#include "Physics/ColliderComponent.hpp"
#include "Physics/Geometry.hpp"
#include "Scene/Actor.hpp"
#include "Scene/Scene.hpp"

#include <glm/geometric.hpp>

namespace cinder::Physics {

using cinder::Scene::Actor;

bool World::raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance,
                    RayHit& hit) const {
    const float reach = glm::length(direction);
    if (reach <= 0.0f || maxDistance <= 0.0f) return false;

    const glm::vec3 heading = direction / reach;
    bool found = false;
    for (Actor* root : scene_.roots()) cast(*root, nullptr, origin, heading, maxDistance, hit, found);
    return found;
}

void World::cast(Actor& actor, RigidBodyComponent* owner, const glm::vec3& origin, const glm::vec3& direction,
                 float maxDistance, RayHit& hit, bool& found) const {
    if (actor.destroyed() || !actor.isEnabled()) return;

    auto* body = actor.findComponent<RigidBodyComponent>();
    if (body != nullptr && body->activeInActor() && body->transform() != nullptr) owner = body;

    for (cinder::Scene::Component* component : actor.components()) {
        auto* collider = dynamic_cast<ColliderComponent*>(component);
        if (collider == nullptr || collider->destroyed() || !collider->activeInActor()) continue;

        float distance = 0.0f;
        glm::vec3 normal(0.0f);
        const Geometry geometry = collider->geometry();
        if (rayHits(geometry, origin, direction, distance, normal) && distance <= maxDistance &&
            (!found || distance < hit.distance)) {
            hit.actor = owner != nullptr ? owner->owner() : &actor;
            hit.collider = collider;
            hit.position = origin + direction * distance;
            hit.normal = normal;
            hit.distance = distance;
            found = true;
        }
    }

    for (Actor* child : actor.attachedActors()) cast(*child, owner, origin, direction, maxDistance, hit, found);
}

}
