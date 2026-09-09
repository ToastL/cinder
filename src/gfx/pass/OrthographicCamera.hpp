#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace cinder::gfx::pass {

class OrthographicCamera {
public:
    void setVirtualSize(float width, float height);
    void setPosition(float x, float y);
    void setZoom(float zoom);
    void setRotation(float rotation);

    float virtualWidth() const { return virtualWidth_; }
    float virtualHeight() const { return virtualHeight_; }
    float zoom() const { return zoom_; }

    const glm::mat4& viewProjection();
    glm::vec3 screenToWorld(float screenX, float screenY, float viewWidth, float viewHeight);

private:
    void recompute();

    float virtualWidth_ = 1.0f;
    float virtualHeight_ = 1.0f;
    float x_ = 0.0f;
    float y_ = 0.0f;
    float zoom_ = 1.0f;
    float rotation_ = 0.0f;

    glm::mat4 combined_{1.0f};
    bool dirty_ = true;
};

}
