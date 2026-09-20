#include "physics/Body.hpp"

#include "scene/Transform.hpp"

#include <glm/geometric.hpp>

namespace cinder::physics {

glm::vec3 Body::centre() {
    return simulated_ ? center_ : glm::vec3(transform()->world()[3]);
}

Body& Body::wake() {
    asleep_ = false;
    sleepTimer_ = 0.0f;
    return *this;
}

Body& Body::applyImpulse(const glm::vec3& impulse) {
    impulse_ += impulse;
    return wake();
}

Body& Body::applyImpulse(const glm::vec3& impulse, const glm::vec3& point) {
    impulse_ += impulse;
    angularImpulse_ += glm::cross(point - centre(), impulse);
    return wake();
}

Body& Body::applyForce(const glm::vec3& force) {
    force_ += force;
    return wake();
}

Body& Body::applyForce(const glm::vec3& force, const glm::vec3& point) {
    force_ += force;
    torque_ += glm::cross(point - centre(), force);
    return wake();
}

Body& Body::applyTorque(const glm::vec3& torque) {
    torque_ += torque;
    return wake();
}

}
