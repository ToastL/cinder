#include "dev/EditorCamera.hpp"

#include "components/Camera.hpp"
#include "scene/Transform.hpp"

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/trigonometric.hpp>

#include <algorithm>
#include <cmath>

namespace cinder::dev {
namespace {

constexpr float LOOK_RADIANS_PER_POINT = 0.003f;
constexpr float MAX_PITCH = 1.55f;
constexpr float FAST_MULTIPLIER = 4.0f;
constexpr float SPEED_STEP = 1.2f;
constexpr float MIN_SPEED = 0.25f;
constexpr float MAX_SPEED = 250.0f;
constexpr float DOLLY_SECONDS = 0.2f;
constexpr float PAN_SECONDS_PER_POINT = 0.004f;

}

void EditorCamera::seed(cinder::components::Camera* camera) {
    seeded_ = true;
    if (camera != nullptr) {
        const glm::mat4& world = camera->transform().world();
        const glm::vec3 forward = glm::normalize(-glm::vec3(world[2]));
        position_ = glm::vec3(world[3]);
        pitch_ = std::asin(std::clamp(forward.y, -1.0f, 1.0f));
        yaw_ = std::atan2(-forward.x, -forward.z);
        camera_.setFov(glm::radians(camera->fov()));
        camera_.setClip(camera->nearClip(), camera->farClip());
    }
    place();
}

void EditorCamera::update(const Controls& controls, float dt) {
    if (controls.look != glm::vec2(0.0f)) {
        yaw_ -= controls.look.x * LOOK_RADIANS_PER_POINT;
        pitch_ = std::clamp(pitch_ - controls.look.y * LOOK_RADIANS_PER_POINT, -MAX_PITCH, MAX_PITCH);
    }

    const glm::mat4 rotation = orientation();
    const glm::vec3 right(rotation[0]);
    const glm::vec3 up(rotation[1]);
    const glm::vec3 forward = -glm::vec3(rotation[2]);

    if (controls.looking) {
        speed_ = std::clamp(speed_ * std::pow(SPEED_STEP, controls.scroll), MIN_SPEED, MAX_SPEED);
    } else {
        position_ += forward * (controls.scroll * speed_ * DOLLY_SECONDS);
    }

    glm::vec3 move = controls.move;
    if (glm::dot(move, move) > 1.0f) move = glm::normalize(move);
    const float velocity = speed_ * (controls.fast ? FAST_MULTIPLIER : 1.0f) * dt;
    position_ += (right * move.x + glm::vec3(0.0f, move.y, 0.0f) + forward * move.z) * velocity;
    position_ += (up * controls.pan.y - right * controls.pan.x) * (speed_ * PAN_SECONDS_PER_POINT);

    place();
}

glm::mat4 EditorCamera::orientation() const {
    const glm::mat4 yaw = glm::rotate(glm::mat4(1.0f), yaw_, glm::vec3(0.0f, 1.0f, 0.0f));
    return glm::rotate(yaw, pitch_, glm::vec3(1.0f, 0.0f, 0.0f));
}

void EditorCamera::place() {
    camera_.setWorld(glm::translate(glm::mat4(1.0f), position_) * orientation());
}

}
