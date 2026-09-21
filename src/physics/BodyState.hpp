#pragma once

#include <glm/mat3x3.hpp>
#include <glm/vec3.hpp>

namespace cinder::physics {

class Body;

struct BodyState {
    Body* body = nullptr;
    glm::vec3 center{0.0f};
    glm::vec3 velocity{0.0f};
    glm::vec3 angularVelocity{0.0f};
    glm::vec3 drift{0.0f};
    glm::vec3 angularDrift{0.0f};
    glm::vec3 linearMask{1.0f};
    float inverseMass = 0.0f;
    glm::mat3 inverseInertia{0.0f};
    float volume = 0.0f;
    glm::vec3 moment{0.0f};
    glm::mat3 inertia{0.0f};
};

}
