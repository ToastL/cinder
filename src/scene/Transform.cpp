#include "scene/Transform.hpp"

#include "scene/Actor.hpp"

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace cinder::scene {

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
    Actor* parent = actor_ != nullptr ? actor_->parent() : nullptr;

    if (parent == nullptr) {
        if (worldDirty_) {
            world_ = local();
            worldDirty_ = false;
            version_++;
        }
        return world_;
    }

    Transform& above = parent->transform();
    const glm::mat4& parentWorld = above.world();

    if (worldDirty_ || above.version_ != parentVersion_) {
        world_ = parentWorld * local();
        parentVersion_ = above.version_;
        worldDirty_ = false;
        version_++;
    }
    return world_;
}

glm::vec3 Transform::worldPosition() { return glm::vec3(world()[3]); }

glm::vec3 Transform::forward() { return glm::normalize(-glm::vec3(world()[2])); }

glm::vec3 Transform::right() { return glm::normalize(glm::vec3(world()[0])); }

glm::vec3 Transform::up() { return glm::normalize(glm::vec3(world()[1])); }

}
