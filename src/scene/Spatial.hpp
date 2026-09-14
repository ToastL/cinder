#pragma once

#include "scene/Node.hpp"
#include "scene/Transform.hpp"

namespace cinder::scene {

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
