#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <optional>

namespace cinder::scene {
class Node;
class Scene;
}

namespace cinder::dev {

struct Ray {
    glm::vec3 origin{0.0f};
    glm::vec3 direction{0.0f, 0.0f, -1.0f};
};

struct PickView {
    glm::mat4 viewProjection{1.0f};
    glm::vec2 point{0.0f};
    glm::vec2 size{1.0f};
    glm::vec2 world2d{0.0f};
};

struct SpriteRect {
    glm::vec2 centre{0.0f};
    glm::vec2 half{0.0f};
    float rotation = 0.0f;
};

SpriteRect spriteRect(const glm::mat4& world, glm::vec2 size);
Ray rayThrough(const glm::mat4& viewProjection, glm::vec2 point, glm::vec2 size);
std::optional<float> hitCube(const Ray& ray, const glm::mat4& world);
bool hitSprite(glm::vec2 point, const glm::mat4& world, glm::vec2 size);
cinder::scene::Node* pick(cinder::scene::Scene& scene, const PickView& view);

}
