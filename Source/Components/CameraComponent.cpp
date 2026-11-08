#include "Components/CameraComponent.hpp"

#include "Scene/DrawList.hpp"

namespace cinder::Components {

cinder::Scene::View CameraComponent::view() {
    cinder::Scene::View view;
    view.world = transform().world();
    view.orthographic = projection_ == Projection::Orthographic;
    view.fovDegrees = fov_;
    view.nearClip = near_;
    view.farClip = far_;
    view.zoom = zoom_;
    view.size = virtualSize_;
    return view;
}

void CameraComponent::onRender(float alpha, cinder::Scene::DrawList& draws) {
    draws.background(clearColor_.r, clearColor_.g, clearColor_.b);
    cinder::Scene::View seen = view();
    seen.world = transform().interpolated(alpha);
    draws.camera(seen);
}

}
