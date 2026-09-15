#include "components/Spin.hpp"

#include "scene/Transform.hpp"

namespace cinder::components {

void Spin::onUpdate(float dt) {
    cinder::scene::Node* target = parent();
    cinder::scene::Transform* transform = target != nullptr ? target->transform() : nullptr;
    if (transform != nullptr) transform->rotate(speed_.x * dt, speed_.y * dt, speed_.z * dt);
}

}
