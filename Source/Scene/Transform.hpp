#pragma once

#include "Reflect/Reflect.hpp"

#include <cstdint>
#include <limits>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace cinder::Scene {

class SceneComponent;

class Transform : public cinder::Reflect::PropSink {
public:
    explicit Transform(SceneComponent* owner) : owner_(owner) {}

    SceneComponent* component() const { return owner_; }

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

    void propChanged(const cinder::Reflect::PropDef& prop) override { dirty(); }

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
    SceneComponent* owner_ = nullptr;

    glm::vec3 position_{0.0f};
    glm::vec3 rotation_{0.0f};
    glm::vec3 scale_{1.0f};

    glm::mat4 local_{1.0f};
    glm::mat4 world_{1.0f};

    bool localDirty_ = true;
    bool worldDirty_ = true;
    std::uint64_t version_ = 0;
    std::uint64_t parentVersion_ = std::numeric_limits<std::uint64_t>::max();
};

}
