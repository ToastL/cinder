#include "gfx/pass/ViewCamera.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/matrix.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec4.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace cinder::gfx::pass {
namespace {

constexpr float MIN_ZOOM = 1e-4f;
constexpr float PARALLEL = 1e-8f;

}

void ViewCamera::setView(const cinder::scene::View& view) {
    view_ = view;
    dirty_ = true;
}

void ViewCamera::setViewSize(float width, float height) {
    width_ = std::max(width, 1.0f);
    height_ = std::max(height, 1.0f);
    dirty_ = true;
}

glm::vec2 ViewCamera::size() const {
    if (view_.size.x > 0.0f && view_.size.y > 0.0f) return view_.size;
    return glm::vec2(width_, height_);
}

glm::vec2 ViewCamera::extent() const { return size() / std::max(view_.zoom, MIN_ZOOM); }

void ViewCamera::recompute() {
    glm::mat4 projection(1.0f);
    if (view_.orthographic) {
        const glm::vec2 half = extent() * 0.5f;
        projection = glm::ortho(-half.x, half.x, -half.y, half.y, view_.nearClip, view_.farClip);
    } else {
        projection = glm::perspective(glm::radians(view_.fovDegrees), width_ / height_, view_.nearClip,
                                      view_.farClip);
    }
    projection[1][1] *= -1.0f;
    combined_ = projection * glm::inverse(view_.world);
    dirty_ = false;
}

const glm::mat4& ViewCamera::viewProjection() {
    if (dirty_) recompute();
    return combined_;
}

std::array<glm::vec3, 8> ViewCamera::corners() const {
    const float tanHalfFov = std::tan(glm::radians(view_.fovDegrees) * 0.5f);
    const float depths[2] = {view_.nearClip, view_.farClip};
    const glm::vec2 signs[4] = {{-1.0f, -1.0f}, {1.0f, -1.0f}, {1.0f, 1.0f}, {-1.0f, 1.0f}};

    std::array<glm::vec3, 8> points{};
    for (std::size_t plane = 0; plane < 2; ++plane) {
        const float depth = depths[plane];
        const glm::vec2 half = view_.orthographic
                ? extent() * 0.5f
                : glm::vec2(depth * tanHalfFov * width_ / height_, depth * tanHalfFov);
        for (std::size_t i = 0; i < 4; ++i) {
            points[plane * 4 + i] = glm::vec3(view_.world * glm::vec4(signs[i] * half, -depth, 1.0f));
        }
    }
    return points;
}

glm::vec3 ViewCamera::screenToWorld(float x, float y) {
    const glm::vec2 ndc(x / width_ * 2.0f - 1.0f, y / height_ * 2.0f - 1.0f);
    const glm::mat4 inverse = glm::inverse(viewProjection());

    glm::vec4 front = inverse * glm::vec4(ndc, 0.0f, 1.0f);
    glm::vec4 back = inverse * glm::vec4(ndc, 1.0f, 1.0f);
    front /= front.w;
    back /= back.w;

    const glm::vec3 direction(back - front);
    if (std::abs(direction.z) < PARALLEL) return glm::vec3(front);
    return glm::vec3(front) - direction * (front.z / direction.z);
}

}
