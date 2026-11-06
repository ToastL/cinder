#pragma once

#include <glm/mat2x2.hpp>
#include <glm/matrix.hpp>
#include <glm/vec2.hpp>

namespace cinder::ui {

struct Transform2D {
    glm::mat2 linear{1.0f};
    glm::vec2 translation{0.0f};

    static Transform2D translate(glm::vec2 offset) { return {glm::mat2(1.0f), offset}; }
    static Transform2D scale(float factor) { return {glm::mat2(factor), glm::vec2(0.0f)}; }

    glm::vec2 apply(glm::vec2 point) const { return linear * point + translation; }

    Transform2D then(const Transform2D& outer) const {
        return {outer.linear * linear, outer.linear * translation + outer.translation};
    }

    Transform2D inverse() const {
        const glm::mat2 inverted = glm::inverse(linear);
        return {inverted, -(inverted * translation)};
    }

    bool identity() const { return linear == glm::mat2(1.0f) && translation == glm::vec2(0.0f); }

    bool operator==(const Transform2D&) const = default;
};

}
