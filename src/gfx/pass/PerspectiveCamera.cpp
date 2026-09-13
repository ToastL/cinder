#include "gfx/pass/PerspectiveCamera.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/vec2.hpp>

#include <cmath>
#include <cstddef>

namespace cinder::gfx::pass {

void PerspectiveCamera::setWorld(const glm::mat4& world) {
    world_ = world;
    dirty_ = true;
}

void PerspectiveCamera::setFov(float radians) {
    fov_ = radians;
    dirty_ = true;
}

void PerspectiveCamera::setAspect(float aspect) {
    aspect_ = aspect;
    dirty_ = true;
}

void PerspectiveCamera::setClip(float near, float far) {
    near_ = near;
    far_ = far;
    dirty_ = true;
}

void PerspectiveCamera::recompute() {
    glm::mat4 projection = glm::perspective(fov_, aspect_, near_, far_);
    projection[1][1] *= -1.0f;
    combined_ = projection * glm::inverse(world_);
    dirty_ = false;
}

const glm::mat4& PerspectiveCamera::viewProjection() {
    if (dirty_) recompute();
    return combined_;
}

std::array<glm::vec3, 8> PerspectiveCamera::corners() const {
    const float tanHalfFov = std::tan(fov_ * 0.5f);
    const float depths[2] = {near_, far_};
    const glm::vec2 signs[4] = {{-1.0f, -1.0f}, {1.0f, -1.0f}, {1.0f, 1.0f}, {-1.0f, 1.0f}};

    std::array<glm::vec3, 8> points{};
    for (std::size_t plane = 0; plane < 2; ++plane) {
        const float depth = depths[plane];
        const glm::vec2 half(depth * tanHalfFov * aspect_, depth * tanHalfFov);
        for (std::size_t i = 0; i < 4; ++i) {
            points[plane * 4 + i] = glm::vec3(world_ * glm::vec4(signs[i] * half, -depth, 1.0f));
        }
    }
    return points;
}

}
