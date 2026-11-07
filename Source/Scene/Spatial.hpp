#pragma once

#include "Scene/Node.hpp"
#include "Scene/Transform.hpp"

namespace cinder::Scene {

class Spatial : public Node {
public:
    Spatial() : transform_(this) {}

    Transform* transform() override { return &transform_; }
    const Transform* transform() const override { return &transform_; }

    CINDER_PROPS(Spatial, Node) {}

private:
    Transform transform_;
};

}
