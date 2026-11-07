#include "Nodes/Camera.hpp"

#include "Scene/DrawList.hpp"

namespace cinder::Nodes {

cinder::Scene::View Camera::view() {
    cinder::Scene::View view;
    view.world = transform()->world();
    view.orthographic = projection_ == Projection::Orthographic;
    view.fovDegrees = fov_;
    view.nearClip = near_;
    view.farClip = far_;
    view.zoom = zoom_;
    view.size = virtualSize_;
    return view;
}

void Camera::onRender(float alpha, cinder::Scene::DrawList& draws) {
    draws.background(clearColor_.r, clearColor_.g, clearColor_.b);
    draws.camera(view());
}

}
