#pragma once

#include <glm/common.hpp>
#include <glm/vec2.hpp>

namespace cinder::ui {

struct Rect {
    glm::vec2 min{0.0f};
    glm::vec2 max{0.0f};

    static Rect fromSize(glm::vec2 position, glm::vec2 size) { return {position, position + size}; }

    glm::vec2 size() const { return max - min; }
    float width() const { return max.x - min.x; }
    float height() const { return max.y - min.y; }
    glm::vec2 center() const { return (min + max) * 0.5f; }
    bool empty() const { return max.x <= min.x || max.y <= min.y; }

    bool contains(glm::vec2 point) const {
        return point.x >= min.x && point.y >= min.y && point.x < max.x && point.y < max.y;
    }

    Rect intersect(const Rect& other) const {
        Rect result{glm::max(min, other.min), glm::min(max, other.max)};
        result.max = glm::max(result.max, result.min);
        return result;
    }

    Rect inset(float left, float top, float right, float bottom) const {
        return {min + glm::vec2(left, top), max - glm::vec2(right, bottom)};
    }

    Rect inset(float amount) const { return inset(amount, amount, amount, amount); }
    Rect offset(glm::vec2 delta) const { return {min + delta, max + delta}; }

    bool operator==(const Rect&) const = default;
};

}
