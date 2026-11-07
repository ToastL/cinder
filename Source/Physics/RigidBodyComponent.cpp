#include "Physics/RigidBodyComponent.hpp"

#include "Scene/Actor.hpp"
#include "Scene/SceneComponent.hpp"
#include "Scene/Transform.hpp"

#include <glm/geometric.hpp>

namespace cinder::Physics {

cinder::Scene::Transform* RigidBodyComponent::transform() const {
    return owner() != nullptr ? owner()->transform() : nullptr;
}

cinder::Scene::Transform* RigidBodyComponent::parentTransform() const {
    cinder::Scene::SceneComponent* root = owner() != nullptr ? owner()->rootComponent() : nullptr;
    return root != nullptr ? root->parentTransform() : nullptr;
}

glm::vec3 RigidBodyComponent::center() {
    if (simulated_) return center_;
    cinder::Scene::Transform* placed = transform();
    return placed != nullptr ? glm::vec3(placed->world()[3]) : glm::vec3(0.0f);
}

RigidBodyComponent& RigidBodyComponent::wake() {
    asleep_ = false;
    sleepTimer_ = 0.0f;
    return *this;
}

RigidBodyComponent& RigidBodyComponent::applyImpulse(const glm::vec3& impulse) {
    impulse_ += impulse;
    return wake();
}

RigidBodyComponent& RigidBodyComponent::applyImpulse(const glm::vec3& impulse, const glm::vec3& point) {
    impulse_ += impulse;
    angularImpulse_ += glm::cross(point - center(), impulse);
    return wake();
}

RigidBodyComponent& RigidBodyComponent::applyForce(const glm::vec3& force) {
    force_ += force;
    return wake();
}

RigidBodyComponent& RigidBodyComponent::applyForce(const glm::vec3& force, const glm::vec3& point) {
    force_ += force;
    torque_ += glm::cross(point - center(), force);
    return wake();
}

RigidBodyComponent& RigidBodyComponent::applyTorque(const glm::vec3& torque) {
    torque_ += torque;
    return wake();
}

}
