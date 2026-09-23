#pragma once

#include "ui/core/Rect.hpp"

#include <glm/vec2.hpp>

namespace cinder::ui {

struct Geometry {
    glm::vec2 size{0.0f};
    glm::vec2 position{0.0f};
    float scale = 1.0f;

    static Geometry root(glm::vec2 size) { return {size, glm::vec2(0.0f), 1.0f}; }

    Rect rect() const { return Rect::fromSize(position, size * scale); }
    glm::vec2 local(glm::vec2 absolute) const { return (absolute - position) / scale; }
    glm::vec2 absolute(glm::vec2 local) const { return position + local * scale; }
    bool contains(glm::vec2 absolute) const { return rect().contains(absolute); }

    Geometry child(glm::vec2 offset, glm::vec2 childSize, float childScale = 1.0f) const {
        return {childSize, position + offset * scale, scale * childScale};
    }

    Rect localRect(glm::vec2 offset, glm::vec2 extent) const {
        return Rect::fromSize(absolute(offset), extent * scale);
    }

    bool operator==(const Geometry&) const = default;
};

}
