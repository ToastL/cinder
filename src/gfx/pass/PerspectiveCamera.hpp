#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <array>

namespace cinder::gfx::pass {

class PerspectiveCamera {
public:
    void setWorld(const glm::mat4& world);
    void setFov(float radians);
    void setAspect(float aspect);
    void setClip(float near, float far);

    const glm::mat4& viewProjection();
    std::array<glm::vec3, 8> corners() const;

private:
    void recompute();

    glm::mat4 world_{1.0f};
    glm::mat4 combined_{1.0f};
    float fov_ = 1.0471976f;
    float aspect_ = 16.0f / 9.0f;
    float near_ = 0.1f;
    float far_ = 500.0f;
    bool dirty_ = true;
};

}
