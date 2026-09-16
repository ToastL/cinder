#pragma once

#include "scene/View.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <array>

namespace cinder::gfx::pass {

class ViewCamera {
public:
    void setView(const cinder::scene::View& view);
    void setViewSize(float width, float height);

    const cinder::scene::View& view() const { return view_; }
    glm::vec2 size() const;
    glm::vec2 extent() const;

    const glm::mat4& viewProjection();
    std::array<glm::vec3, 8> corners() const;
    glm::vec3 screenToWorld(float x, float y);

private:
    void recompute();

    cinder::scene::View view_;
    float width_ = 1.0f;
    float height_ = 1.0f;
    glm::mat4 combined_{1.0f};
    bool dirty_ = true;
};

}
