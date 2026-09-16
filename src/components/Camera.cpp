#include "components/Camera.hpp"

#include "scene/DrawList.hpp"

namespace cinder::components {

cinder::scene::View Camera::view() {
    cinder::scene::View view;
    view.world = transform()->world();
    view.orthographic = projection_ == Projection::Orthographic;
    view.fovDegrees = fov_;
    view.nearClip = near_;
    view.farClip = far_;
    view.zoom = zoom_;
    view.size = virtualSize_;
    return view;
}

void Camera::onRender(float alpha, cinder::scene::DrawList& draws) {
    draws.background(clearColor_.r, clearColor_.g, clearColor_.b);
    draws.camera(view());
}

}
