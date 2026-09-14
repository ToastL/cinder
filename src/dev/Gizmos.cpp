#include "dev/Gizmos.hpp"

#include "components/Camera.hpp"
#include "gfx/pass/PerspectiveCamera.hpp"
#include "scene/Node.hpp"
#include "scene/Scene.hpp"
#include "scene/Transform.hpp"

#include <imgui.h>

#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec4.hpp>

#include <algorithm>
#include <array>
#include <cstddef>

namespace cinder::dev {

using cinder::components::Camera;

namespace {

constexpr ImU32 EDGE = IM_COL32(235, 235, 235, 255);
constexpr ImU32 SIGHT = IM_COL32(235, 235, 235, 110);
constexpr float THICKNESS = 1.5f;
constexpr float INSET = 0.999f;

const std::array<glm::vec4, 5> CLIP_PLANES = {
    glm::vec4(1.0f, 0.0f, 0.0f, INSET),
    glm::vec4(-1.0f, 0.0f, 0.0f, INSET),
    glm::vec4(0.0f, 1.0f, 0.0f, INSET),
    glm::vec4(0.0f, -1.0f, 0.0f, INSET),
    glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
};

struct Screen {
    glm::mat4 viewProjection;
    glm::vec2 origin;
    glm::vec2 size;
};

bool clipSegment(glm::vec4& from, glm::vec4& to) {
    float enter = 0.0f;
    float exit = 1.0f;
    for (const glm::vec4& plane : CLIP_PLANES) {
        const float a = glm::dot(plane, from);
        const float b = glm::dot(plane, to);
        if (a < 0.0f && b < 0.0f) return false;
        if (a < 0.0f) enter = std::max(enter, a / (a - b));
        else if (b < 0.0f) exit = std::min(exit, a / (a - b));
    }
    if (enter > exit) return false;

    const glm::vec4 delta = to - from;
    to = from + delta * exit;
    from = from + delta * enter;
    return true;
}

ImVec2 project(const Screen& screen, const glm::vec4& position) {
    const glm::vec2 ndc = glm::vec2(position) / position.w;
    const glm::vec2 point = screen.origin + (ndc * 0.5f + 0.5f) * screen.size;
    return ImVec2(point.x, point.y);
}

void segment(ImDrawList& list, const Screen& screen, const glm::vec3& from, const glm::vec3& to,
             ImU32 color) {
    glm::vec4 a = screen.viewProjection * glm::vec4(from, 1.0f);
    glm::vec4 b = screen.viewProjection * glm::vec4(to, 1.0f);
    if (clipSegment(a, b)) list.AddLine(project(screen, a), project(screen, b), color, THICKNESS);
}

void frustum(ImDrawList& list, const Screen& screen, Camera& camera) {
    cinder::scene::Transform& transform = *camera.transform();

    cinder::gfx::pass::PerspectiveCamera view;
    view.setWorld(transform.world());
    view.setFov(glm::radians(camera.fov()));
    view.setAspect(screen.size.x / screen.size.y);
    view.setClip(camera.nearClip(), camera.farClip());

    const std::array<glm::vec3, 8> corners = view.corners();
    const glm::vec3 eye = transform.worldPosition();
    for (std::size_t i = 0; i < 4; ++i) {
        const std::size_t next = (i + 1) % 4;
        segment(list, screen, eye, corners[i], SIGHT);
        segment(list, screen, corners[i], corners[next], EDGE);
        segment(list, screen, corners[i + 4], corners[next + 4], EDGE);
        segment(list, screen, corners[i], corners[i + 4], EDGE);
    }
}

void collect(cinder::scene::Node& node, std::vector<Camera*>& cameras) {
    if (node.destroyed() || !node.isEnabled()) return;

    auto* camera = dynamic_cast<Camera*>(&node);
    if (camera != nullptr && camera->projection() == Camera::Projection::Perspective) {
        cameras.push_back(camera);
    }
    for (cinder::scene::Node* child : node.children()) collect(*child, cameras);
}

}

std::vector<Camera*> perspectiveCameras(cinder::scene::Scene& scene) {
    std::vector<Camera*> cameras;
    for (cinder::scene::Node* root : scene.roots()) collect(*root, cameras);
    return cameras;
}

void drawFrustums(ImDrawList& list, const std::vector<Camera*>& cameras,
                  const glm::mat4& viewProjection, glm::vec2 origin, glm::vec2 size) {
    const Screen screen{viewProjection, origin, size};
    list.PushClipRect(ImVec2(origin.x, origin.y), ImVec2(origin.x + size.x, origin.y + size.y), true);
    for (Camera* camera : cameras) frustum(list, screen, *camera);
    list.PopClipRect();
}

}
