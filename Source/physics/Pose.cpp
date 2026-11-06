#include "physics/Pose.hpp"

#include "physics/Geometry.hpp"

#include <glm/geometric.hpp>
#include <glm/gtx/euler_angles.hpp>

#include <cmath>

namespace cinder::physics {

glm::quat orientationOf(const glm::mat4& world) {
    return glm::normalize(glm::quat_cast(axesOf(world)));
}

glm::vec3 rotationOf(const glm::quat& orientation) {
    float yaw = 0.0f;
    float pitch = 0.0f;
    float roll = 0.0f;
    glm::extractEulerAngleYXZ(glm::mat4_cast(orientation), yaw, pitch, roll);
    return glm::vec3(pitch, yaw, roll);
}

glm::vec3 angularVelocityOf(const glm::quat& from, const glm::quat& to, float dt) {
    glm::quat delta = to * glm::conjugate(from);
    if (delta.w < 0.0f) delta = -delta;

    const glm::vec3 axis(delta.x, delta.y, delta.z);
    const float sine = glm::length(axis);
    if (sine <= 0.0f || dt <= 0.0f) return glm::vec3(0.0f);
    return axis / sine * (2.0f * std::atan2(sine, delta.w) / dt);
}

}
