#include "components/Camera.hpp"

#include "scene/DrawList.hpp"

namespace cinder::components {

void Camera::onRender(float alpha, cinder::scene::DrawList& draws) {
    cinder::scene::Transform& t = *transform();
    draws.background(clearColor_.r, clearColor_.g, clearColor_.b);

    if (projection_ == Projection::Perspective) {
        draws.camera3d(t.world(), fov_, near_, far_);
        return;
    }

    const glm::vec3 position = t.worldPosition();
    draws.camera2d(position.x, position.y, zoom_, t.rotation().z, virtualSize_.x, virtualSize_.y);
}

}
