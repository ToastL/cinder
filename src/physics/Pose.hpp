#pragma once

#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace cinder::physics {

glm::quat orientationOf(const glm::mat4& world);
glm::vec3 rotationOf(const glm::quat& orientation);
glm::vec3 angularVelocityOf(const glm::quat& from, const glm::quat& to, float dt);

}
