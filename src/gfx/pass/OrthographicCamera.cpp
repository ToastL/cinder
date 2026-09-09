#include "gfx/pass/OrthographicCamera.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace cinder::gfx::pass {

void OrthographicCamera::setVirtualSize(float width, float height) {
    virtualWidth_ = width;
    virtualHeight_ = height;
    dirty_ = true;
}

void OrthographicCamera::setPosition(float x, float y) {
    x_ = x;
    y_ = y;
    dirty_ = true;
}

void OrthographicCamera::setZoom(float zoom) {
    zoom_ = zoom;
    dirty_ = true;
}

void OrthographicCamera::setRotation(float rotation) {
    rotation_ = rotation;
    dirty_ = true;
}

void OrthographicCamera::recompute() {
    const float halfWidth = virtualWidth_ * 0.5f;
    const float halfHeight = virtualHeight_ * 0.5f;

    combined_ = glm::ortho(0.0f, virtualWidth_, 0.0f, virtualHeight_, -1.0f, 1.0f);
    combined_ = glm::translate(combined_, glm::vec3(halfWidth, halfHeight, 0.0f));
    combined_ = glm::rotate(combined_, -rotation_, glm::vec3(0.0f, 0.0f, 1.0f));
    combined_ = glm::scale(combined_, glm::vec3(zoom_));
    combined_ = glm::translate(combined_, glm::vec3(-x_ - halfWidth, -y_ - halfHeight, 0.0f));

    dirty_ = false;
}

const glm::mat4& OrthographicCamera::viewProjection() {
    if (dirty_) recompute();
    return combined_;
}

glm::vec3 OrthographicCamera::screenToWorld(float screenX, float screenY,
                                            float viewWidth, float viewHeight) {
    const glm::mat4 inverse = glm::inverse(viewProjection());
    const glm::vec4 ndc(screenX / viewWidth * 2.0f - 1.0f,
                        screenY / viewHeight * 2.0f - 1.0f, 0.0f, 1.0f);
    const glm::vec4 world = inverse * ndc;
    return glm::vec3(world);
}

}
