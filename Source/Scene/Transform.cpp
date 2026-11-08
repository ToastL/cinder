#include "Scene/Transform.hpp"

#include "Scene/SceneComponent.hpp"

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cmath>

namespace cinder::Scene {
namespace {

constexpr float MIN_SCALE = 1e-6f;
constexpr float MAX_SKEW = 1e-3f;

struct Pose {
    glm::vec3 translation;
    glm::quat rotation;
    glm::vec3 scale;
};

bool decompose(const glm::mat4& matrix, Pose& pose) {
    glm::vec3 x(matrix[0]);
    glm::vec3 y(matrix[1]);
    glm::vec3 z(matrix[2]);
    pose.scale = glm::vec3(glm::length(x), glm::length(y), glm::length(z));
    if (pose.scale.x < MIN_SCALE || pose.scale.y < MIN_SCALE || pose.scale.z < MIN_SCALE) return false;
    x /= pose.scale.x;
    y /= pose.scale.y;
    z /= pose.scale.z;
    if (std::abs(glm::dot(x, y)) > MAX_SKEW || std::abs(glm::dot(y, z)) > MAX_SKEW
        || std::abs(glm::dot(z, x)) > MAX_SKEW) {
        return false;
    }
    if (glm::dot(glm::cross(x, y), z) < 0.0f) {
        x = -x;
        pose.scale.x = -pose.scale.x;
    }
    pose.rotation = glm::quat_cast(glm::mat3(x, y, z));
    pose.translation = glm::vec3(matrix[3]);
    return true;
}

glm::mat4 blend(const glm::mat4& from, const glm::mat4& to, float t) {
    if (from == to) return to;
    Pose a;
    Pose b;
    if (!decompose(from, a) || !decompose(to, b) || (a.scale.x < 0.0f) != (b.scale.x < 0.0f)) {
        return from + (to - from) * t;
    }
    glm::mat4 result = glm::mat4_cast(glm::slerp(a.rotation, b.rotation, t));
    const glm::vec3 scale = glm::mix(a.scale, b.scale, t);
    result[0] *= scale.x;
    result[1] *= scale.y;
    result[2] *= scale.z;
    result[3] = glm::vec4(glm::mix(a.translation, b.translation, t), 1.0f);
    return result;
}

}

Transform& Transform::setPosition(float x, float y, float z) {
    position_ = glm::vec3(x, y, z);
    return dirty();
}

Transform& Transform::setRotation(float x, float y, float z) {
    rotation_ = glm::vec3(x, y, z);
    return dirty();
}

Transform& Transform::setScale(float x, float y, float z) {
    scale_ = glm::vec3(x, y, z);
    return dirty();
}

Transform& Transform::translate(float x, float y, float z) {
    position_ += glm::vec3(x, y, z);
    return dirty();
}

Transform& Transform::rotate(float x, float y, float z) {
    rotation_ += glm::vec3(x, y, z);
    return dirty();
}

Transform& Transform::dirty() {
    localDirty_ = true;
    worldDirty_ = true;
    return *this;
}

const glm::mat4& Transform::local() {
    if (localDirty_) {
        local_ = glm::translate(glm::mat4(1.0f), position_);
        local_ = glm::rotate(local_, rotation_.y, glm::vec3(0.0f, 1.0f, 0.0f));
        local_ = glm::rotate(local_, rotation_.x, glm::vec3(1.0f, 0.0f, 0.0f));
        local_ = glm::rotate(local_, rotation_.z, glm::vec3(0.0f, 0.0f, 1.0f));
        local_ = glm::scale(local_, scale_);
        localDirty_ = false;
    }
    return local_;
}

const glm::mat4& Transform::world() {
    Transform* above = owner_ != nullptr ? owner_->parentTransform() : nullptr;

    if (above == nullptr) {
        if (worldDirty_) {
            world_ = local();
            worldDirty_ = false;
            version_++;
        }
        return world_;
    }

    const glm::mat4& parentWorld = above->world();

    if (worldDirty_ || above->version_ != parentVersion_) {
        world_ = parentWorld * local();
        parentVersion_ = above->version_;
        worldDirty_ = false;
        version_++;
    }
    return world_;
}

void Transform::snapshot() {
    previous_ = world();
    hasPrevious_ = true;
}

glm::mat4 Transform::interpolated(float alpha) {
    const glm::mat4& now = world();
    if (!hasPrevious_ || alpha >= 1.0f) return now;
    return blend(previous_, now, alpha < 0.0f ? 0.0f : alpha);
}

glm::vec3 Transform::worldPosition() { return glm::vec3(world()[3]); }

glm::vec3 Transform::forward() { return glm::normalize(-glm::vec3(world()[2])); }

glm::vec3 Transform::right() { return glm::normalize(glm::vec3(world()[0])); }

glm::vec3 Transform::up() { return glm::normalize(glm::vec3(world()[1])); }

}
