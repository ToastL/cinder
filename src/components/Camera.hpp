#pragma once

#include "scene/Spatial.hpp"

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace cinder::components {

class Camera final : public cinder::scene::Spatial {
public:
    enum class Projection { Perspective, Orthographic };

    Projection projection() const { return projection_; }
    float fov() const { return fov_; }
    float nearClip() const { return near_; }
    float farClip() const { return far_; }
    float zoom() const { return zoom_; }
    const glm::vec4& clearColor() const { return clearColor_; }
    const glm::vec2& virtualSize() const { return virtualSize_; }

    Camera& setProjection(Projection projection) { projection_ = projection; return *this; }
    Camera& setFov(float degrees) { fov_ = degrees; return *this; }
    Camera& setZoom(float zoom) { zoom_ = zoom; return *this; }
    Camera& setClip(float near, float far) { near_ = near; far_ = far; return *this; }
    Camera& setClearColor(float r, float g, float b, float a) {
        clearColor_ = glm::vec4(r, g, b, a);
        return *this;
    }
    Camera& setVirtualSize(float width, float height) {
        virtualSize_ = glm::vec2(width, height);
        return *this;
    }

    void onRender(float alpha, cinder::scene::DrawList& draws) override;

    CINDER_NODE(Camera, cinder::scene::Spatial) {
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

CINDER_ENUM_NAMES(cinder::components::Camera::Projection, "perspective", "orthographic")
