#pragma once

#include "scene/Component.hpp"

#include <glm/vec3.hpp>

namespace cinder::components {

class Spin final : public cinder::scene::Component {
public:
    const glm::vec3& speed() const { return speed_; }

    Spin& setSpeed(float x, float y, float z) { speed_ = glm::vec3(x, y, z); return *this; }

    void onUpdate(float dt) override;

    CINDER_COMPONENT(Spin, cinder::scene::Component) { CINDER_PROP(speed_); }

private:
    glm::vec3 speed_{0.0f, 1.0f, 0.0f};
};

}
