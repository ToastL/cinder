#pragma once

#include "Scene/SceneComponent.hpp"
#include "Scene/View.hpp"

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace cinder::Components {

class CameraComponent final : public cinder::Scene::SceneComponent {
public:
    enum class Projection { Perspective, Orthographic };

    Projection projection() const { return projection_; }
    float fov() const { return fov_; }
    float nearClip() const { return near_; }
    float farClip() const { return far_; }
    float zoom() const { return zoom_; }
    const glm::vec4& clearColor() const { return clearColor_; }
    const glm::vec2& virtualSize() const { return virtualSize_; }

    CameraComponent& setProjection(Projection projection) { projection_ = projection; return *this; }
    CameraComponent& setFov(float degrees) { fov_ = degrees; return *this; }
    CameraComponent& setZoom(float zoom) { zoom_ = zoom; return *this; }
    CameraComponent& setClip(float nearClip, float farClip) { near_ = nearClip; far_ = farClip; return *this; }
    CameraComponent& setClearColor(float r, float g, float b, float a) {
        clearColor_ = glm::vec4(r, g, b, a);
        return *this;
    }
    CameraComponent& setVirtualSize(float width, float height) {
        virtualSize_ = glm::vec2(width, height);
        return *this;
    }

    cinder::Scene::View view();

    void onRender(float alpha, cinder::Scene::DrawList& draws) override;

    CINDER_COMPONENT(CameraComponent, cinder::Scene::SceneComponent) {
        CINDER_PROP(projection_);
        CINDER_PROP_S(fov_, 1.0f, 179.0f, 1.0f);
        CINDER_PROP(near_);
        CINDER_PROP(far_);
        CINDER_PROP_R(zoom_, 0.05f, 20.0f);
        CINDER_PROP_COLOR(clearColor_);
        CINDER_PROP(virtualSize_);
    }

private:
    Projection projection_ = Projection::Perspective;
    float fov_ = 60.0f;
    float near_ = 0.1f;
    float far_ = 500.0f;
    float zoom_ = 1.0f;
    glm::vec4 clearColor_{0.02f, 0.02f, 0.05f, 1.0f};
    glm::vec2 virtualSize_{0.0f, 0.0f};
};

}

CINDER_ENUM_NAMES(cinder::Components::CameraComponent::Projection, "perspective", "orthographic")
