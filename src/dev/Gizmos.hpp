#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>

#include <vector>

struct ImDrawList;

namespace cinder::components { class Camera; }
namespace cinder::scene { class Scene; }

namespace cinder::dev {

std::vector<cinder::components::Camera*> perspectiveCameras(cinder::scene::Scene& scene);

void drawFrustums(ImDrawList& list, const std::vector<cinder::components::Camera*>& cameras,
                  const glm::mat4& viewProjection, glm::vec2 origin, glm::vec2 size);

}
