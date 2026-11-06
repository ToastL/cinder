#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>

namespace cinder::scene {

struct View {
    glm::mat4 world{1.0f};
    bool orthographic = false;
    float fovDegrees = 60.0f;
    float nearClip = 0.1f;
    float farClip = 500.0f;
    float zoom = 1.0f;
    glm::vec2 size{0.0f};
};

}
