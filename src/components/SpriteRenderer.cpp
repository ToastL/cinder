#include "components/SpriteRenderer.hpp"

#include "scene/Actor.hpp"
#include "scene/DrawList.hpp"

#include <cmath>
#include <glm/geometric.hpp>

namespace cinder::components {

void SpriteRenderer::onRender(float alpha, cinder::scene::DrawList& draws) {
    const glm::mat4& world = transform().world();

    const glm::vec3 position(world[3]);
    const float scaleX = glm::length(glm::vec3(world[0]));
    const float scaleY = glm::length(glm::vec3(world[1]));

    const float w = size_.x * scaleX;
    const float h = size_.y * scaleY;

    draws.sprite(texture_,
                 position.x - w * 0.5f, position.y - h * 0.5f, w, h,
                 std::atan2(world[0][1], world[0][0]),
                 color_.x, color_.y, color_.z, color_.w);
}

}
