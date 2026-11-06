#pragma once

#include "physics/Geometry.hpp"
#include "scene/Spatial.hpp"

#include <glm/vec3.hpp>

#include <cfloat>

namespace cinder::physics {

class Collider final : public cinder::scene::Spatial {
public:
    Shape shape() const { return shape_; }
    const glm::vec3& size() const { return size_; }
    float friction() const { return friction_; }
    float restitution() const { return restitution_; }

    Collider& setShape(Shape shape) { shape_ = shape; return *this; }
    Collider& setSize(float x, float y, float z) { size_ = glm::vec3(x, y, z); return *this; }
    Collider& setFriction(float friction) { friction_ = friction; return *this; }
    Collider& setRestitution(float restitution) { restitution_ = restitution; return *this; }

    Geometry geometry();

    CINDER_NODE(Collider, cinder::scene::Spatial) {
        CINDER_PROP(shape_);
        CINDER_PROP_R(size_, 0.0f, FLT_MAX);
        CINDER_PROP_R(friction_, 0.0f, FLT_MAX);
        CINDER_PROP_R(restitution_, 0.0f, 1.0f);
    }

private:
    Shape shape_ = Shape::Box;
    glm::vec3 size_{1.0f};
    float friction_ = 0.5f;
    float restitution_ = 0.0f;
};

}
