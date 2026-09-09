#include "components/Spin.hpp"

#include "scene/Actor.hpp"

namespace cinder::components {

void Spin::onUpdate(float dt) {
    transform().rotate(speed_.x * dt, speed_.y * dt, speed_.z * dt);
}

}
