#pragma once

#include "dev/GizmoLines.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>

#include <vector>

struct ImDrawList;

namespace cinder::dev {

void drawFrustums(ImDrawList& list, const std::vector<cinder::components::Camera*>& cameras,
                  const glm::mat4& viewProjection, glm::vec2 origin, glm::vec2 size);

void drawColliders(ImDrawList& list, cinder::scene::Scene& scene, const glm::mat4& viewProjection,
                   glm::vec2 origin, glm::vec2 size);

void drawSelection(ImDrawList& list, cinder::scene::Node& node, const glm::mat4& viewProjection,
                   glm::vec2 origin, glm::vec2 size);

void drawManipulator(ImDrawList& list, const std::vector<HandleShape>& shapes, Handle hot,
                     const glm::mat4& viewProjection, glm::vec2 origin, glm::vec2 size);

}
