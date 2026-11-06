#pragma once

#include "reflect/Reflect.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace cinder::scene {

class Node;

class Transform : public cinder::reflect::PropSink {
public:
    explicit Transform(Node* node) : node_(node) {}

    Node* node() const { return node_; }

    glm::vec3& position() { return position_; }
    glm::vec3& rotation() { return rotation_; }
    glm::vec3& scale() { return scale_; }

    const glm::vec3& position() const { return position_; }
    const glm::vec3& rotation() const { return rotation_; }
    const glm::vec3& scale() const { return scale_; }

    Transform& setPosition(float x, float y, float z);
    Transform& setRotation(float x, float y, float z);
    Transform& setScale(float x, float y, float z);
    Transform& setScale(float s) { return setScale(s, s, s); }
    Transform& translate(float x, float y, float z);
    Transform& rotate(float x, float y, float z);
    Transform& dirty();

    void propChanged(const cinder::reflect::PropDef& prop) override { dirty(); }

    const glm::mat4& local();
    const glm::mat4& world();

    glm::vec3 worldPosition();
    glm::vec3 forward();
    glm::vec3 right();
    glm::vec3 up();

    CINDER_PROPS(Transform, void) {
        CINDER_PROP(position_);
        CINDER_PROP_ANGLE(rotation_);
        CINDER_PROP(scale_);
    }

private:
    Node* node_ = nullptr;

    glm::vec3 position_{0.0f};
    glm::vec3 rotation_{0.0f};
    glm::vec3 scale_{1.0f};

    glm::mat4 local_{1.0f};
    glm::mat4 world_{1.0f};

    bool localDirty_ = true;
    bool worldDirty_ = true;
    int version_ = 0;
    int parentVersion_ = -1;
};

}
