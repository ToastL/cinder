#pragma once

#include "Dev/GizmoGeometry.hpp"

#include <glm/ext/vector_uint4_sized.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>

#include <vector>

namespace cinder::Components { class CameraComponent; }

namespace cinder::Scene {
class Object;
class Scene;
}

namespace cinder::Dev {

struct GizmoCanvas {
    glm::mat4 viewProjection{1.0f};
    glm::vec2 origin{0.0f};
    glm::vec2 size{1.0f};
};

struct GizmoStroke {
    std::vector<glm::vec2> points;
    glm::u8vec4 color{255};
    float thickness = 0.0f;
    bool closed = false;

    bool filled() const { return thickness <= 0.0f; }
};

using GizmoStrokes = std::vector<GizmoStroke>;

std::vector<cinder::Components::CameraComponent*> sceneCameras(cinder::Scene::Scene& scene);

void frustumStrokes(GizmoStrokes& out, const std::vector<cinder::Components::CameraComponent*>& cameras,
                    const GizmoCanvas& canvas);
void colliderStrokes(GizmoStrokes& out, cinder::Scene::Scene& scene, const GizmoCanvas& canvas);
void selectionStrokes(GizmoStrokes& out, cinder::Scene::Object& object, const GizmoCanvas& canvas);
void manipulatorStrokes(GizmoStrokes& out, const std::vector<HandleShape>& shapes, Handle hot,
                        const GizmoCanvas& canvas);

}
