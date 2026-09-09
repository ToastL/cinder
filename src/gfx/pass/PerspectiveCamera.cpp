#include "gfx/pass/PerspectiveCamera.hpp"

#include <glm/gtc/matrix_transform.hpp>

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

}
