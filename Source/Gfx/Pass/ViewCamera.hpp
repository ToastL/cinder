#pragma once

#include "Scene/View.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <array>

namespace cinder::Gfx::Pass {

class ViewCamera {
public:
    void setView(const cinder::Scene::View& view);
    void setViewSize(float width, float height);

    const cinder::Scene::View& view() const { return view_; }
    glm::vec2 size() const;
    glm::vec2 extent() const;

    const glm::mat4& viewProjection() const;
    std::array<glm::vec3, 8> corners() const;
    glm::vec3 screenToWorld(float x, float y) const;

private:
    void recompute() const;

    cinder::Scene::View view_;
    float width_ = 1.0f;
    float height_ = 1.0f;
    mutable glm::mat4 combined_{1.0f};
    mutable bool dirty_ = true;
};

}
