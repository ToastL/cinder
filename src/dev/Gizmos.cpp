#include "dev/Gizmos.hpp"

#include "components/Camera.hpp"
#include "components/MeshPart.hpp"
#include "components/Sprite.hpp"
#include "gfx/pass/ViewCamera.hpp"
#include "physics/Collider.hpp"
#include "scene/Node.hpp"
#include "scene/Scene.hpp"
#include "scene/Transform.hpp"

#include <imgui.h>

#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/vec4.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <optional>

namespace cinder::dev {

using cinder::components::Camera;

namespace {

constexpr ImU32 EDGE = IM_COL32(235, 235, 235, 255);
constexpr ImU32 SIGHT = IM_COL32(235, 235, 235, 110);
constexpr ImU32 SELECTED = IM_COL32(255, 176, 46, 255);
constexpr ImU32 SELECTED_SIGHT = IM_COL32(255, 176, 46, 110);
constexpr ImU32 AXIS_X = IM_COL32(232, 72, 72, 255);
constexpr ImU32 AXIS_Y = IM_COL32(120, 204, 80, 255);
constexpr ImU32 AXIS_Z = IM_COL32(72, 128, 240, 255);
constexpr ImU32 NEUTRAL = IM_COL32(235, 235, 235, 255);
constexpr ImU32 HOT = IM_COL32(255, 226, 64, 255);
constexpr ImU32 COLLIDER = IM_COL32(110, 220, 130, 200);
constexpr float THICKNESS = 1.5f;
constexpr float HANDLE_THICKNESS = 2.5f;
constexpr float ARROW_LENGTH = 12.0f;
constexpr float ARROW_WIDTH = 5.0f;
constexpr float BOX_HALF = 4.5f;
constexpr int FILL_ALPHA = 80;
constexpr int BACK_ALPHA = 70;
constexpr float INSET = 0.999f;
constexpr float HALF_EXTENT = 0.5f;
constexpr int CIRCLE_SEGMENTS = 48;

const std::array<glm::vec4, 5> CLIP_PLANES = {
    glm::vec4(1.0f, 0.0f, 0.0f, INSET),
    glm::vec4(-1.0f, 0.0f, 0.0f, INSET),
    glm::vec4(0.0f, 1.0f, 0.0f, INSET),
    glm::vec4(0.0f, -1.0f, 0.0f, INSET),
    glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
};

const std::array<glm::vec2, 4> SIGNS = {
    glm::vec2(-1.0f, -1.0f), glm::vec2(1.0f, -1.0f), glm::vec2(1.0f, 1.0f), glm::vec2(-1.0f, 1.0f),
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
             ImU32 color, float thickness = THICKNESS) {
    glm::vec4 a = screen.viewProjection * glm::vec4(from, 1.0f);
    glm::vec4 b = screen.viewProjection * glm::vec4(to, 1.0f);
    if (clipSegment(a, b)) list.AddLine(project(screen, a), project(screen, b), color, thickness);
}

void frustum(ImDrawList& list, const Screen& screen, Camera& camera, ImU32 edge, ImU32 sight) {
    const cinder::scene::View seen = camera.view();
    cinder::gfx::pass::ViewCamera view;
    view.setView(seen);
    view.setViewSize(screen.size.x, screen.size.y);

    const std::array<glm::vec3, 8> corners = view.corners();
    const glm::vec3 eye(seen.world[3]);
    for (std::size_t i = 0; i < 4; ++i) {
        const std::size_t next = (i + 1) % 4;
        segment(list, screen, corners[i], corners[next], edge);
        if (seen.orthographic) continue;

        segment(list, screen, eye, corners[i], sight);
        segment(list, screen, corners[i + 4], corners[next + 4], edge);
        segment(list, screen, corners[i], corners[i + 4], edge);
    }
}

void box(ImDrawList& list, const Screen& screen, const glm::mat4& world, ImU32 color) {
    std::array<glm::vec3, 8> corners{};
    for (std::size_t i = 0; i < corners.size(); ++i) {
        const glm::vec3 local((i & 1) != 0 ? HALF_EXTENT : -HALF_EXTENT, (i & 2) != 0 ? HALF_EXTENT : -HALF_EXTENT,
                              (i & 4) != 0 ? HALF_EXTENT : -HALF_EXTENT);
        corners[i] = glm::vec3(world * glm::vec4(local, 1.0f));
    }
    for (std::size_t i = 0; i < corners.size(); ++i) {
        for (std::size_t bit = 1; bit < corners.size(); bit <<= 1) {
            if ((i & bit) == 0) segment(list, screen, corners[i], corners[i | bit], color);
        }
    }
}

void outline(ImDrawList& list, const Screen& screen, const glm::mat4& world, glm::vec2 size) {
    const glm::vec2 half = size * 0.5f;
    std::array<glm::vec3, 4> corners{};
    for (std::size_t i = 0; i < corners.size(); ++i) {
        corners[i] = glm::vec3(world * glm::vec4(SIGNS[i] * half, 0.0f, 1.0f));
    }
    for (std::size_t i = 0; i < corners.size(); ++i) {
        segment(list, screen, corners[i], corners[(i + 1) % corners.size()], SELECTED);
    }
}

void ball(ImDrawList& list, const Screen& screen, const glm::mat3& axes, const glm::vec3& centre,
          float radius, ImU32 color) {
    for (int axis = 0; axis < 3; ++axis) {
        const glm::vec3 u = axes[axis] * radius;
        const glm::vec3 v = axes[(axis + 1) % 3] * radius;
        glm::vec3 previous = centre + u;
        for (int i = 1; i <= CIRCLE_SEGMENTS; ++i) {
            const float angle = glm::two_pi<float>() * static_cast<float>(i) / CIRCLE_SEGMENTS;
            const glm::vec3 next = centre + u * std::cos(angle) + v * std::sin(angle);
            segment(list, screen, previous, next, color);
            previous = next;
        }
    }
}

void shape(ImDrawList& list, const Screen& screen, const cinder::physics::Geometry& geometry, ImU32 color) {
    if (geometry.shape == cinder::physics::Shape::Box) {
        glm::mat4 world(1.0f);
        for (int i = 0; i < 3; ++i) world[i] = glm::vec4(geometry.axes[i] * (2.0f * geometry.halfExtents[i]), 0.0f);
        world[3] = glm::vec4(geometry.center, 1.0f);
        box(list, screen, world, color);
        return;
    }

    const float half = cinder::physics::segmentHalf(geometry);
    const glm::vec3 along = cinder::physics::segmentAxis(geometry) * half;
    ball(list, screen, geometry.axes, geometry.center - along, geometry.radius, color);
    if (half <= 0.0f) return;

    ball(list, screen, geometry.axes, geometry.center + along, geometry.radius, color);
    for (const int axis : {0, 2}) {
        for (const float side : {-1.0f, 1.0f}) {
            const glm::vec3 offset = geometry.axes[axis] * (side * geometry.radius);
            segment(list, screen, geometry.center - along + offset, geometry.center + along + offset,
                    color);
        }
    }
}

void colliders(ImDrawList& list, const Screen& screen, cinder::scene::Node& node) {
    if (node.destroyed() || !node.isEnabled()) return;

    if (auto* collider = dynamic_cast<cinder::physics::Collider*>(&node)) {
        shape(list, screen, collider->geometry(), COLLIDER);
    }
    for (cinder::scene::Node* child : node.children()) colliders(list, screen, *child);
}

ImU32 colorOf(Handle handle) {
    switch (handle) {
        case Handle::X:
        case Handle::YZ: return AXIS_X;
        case Handle::Y:
        case Handle::ZX: return AXIS_Y;
        case Handle::Z:
        case Handle::XY: return AXIS_Z;
        default: return NEUTRAL;
    }
}

ImU32 withAlpha(ImU32 color, int alpha) {
    return (color & ~IM_COL32_A_MASK) | (static_cast<ImU32>(alpha) << IM_COL32_A_SHIFT);
}

std::optional<ImVec2> point(const Screen& screen, const glm::vec3& position) {
    const glm::vec4 clip = screen.viewProjection * glm::vec4(position, 1.0f);
    if (clip.w <= 0.0f || clip.z < 0.0f) return std::nullopt;
    return project(screen, clip);
}

void fill(ImDrawList& list, const Screen& screen, const HandleShape& shape, ImU32 color) {
    std::vector<ImVec2> corners;
    for (const glm::vec3& position : shape.points) {
        std::optional<ImVec2> corner = point(screen, position);
        if (!corner) return;
        corners.push_back(*corner);
    }

    float area = 0.0f;
    for (std::size_t i = 0; i < corners.size(); ++i) {
        const ImVec2& a = corners[i];
        const ImVec2& b = corners[(i + 1) % corners.size()];
        area += a.x * b.y - b.x * a.y;
    }
    if (area < 0.0f) std::reverse(corners.begin(), corners.end());

    const int count = static_cast<int>(corners.size());
    list.AddConvexPolyFilled(corners.data(), count, withAlpha(color, FILL_ALPHA));
    list.AddPolyline(corners.data(), count, color, ImDrawFlags_Closed, THICKNESS);
}

void tip(ImDrawList& list, const Screen& screen, const HandleShape& shape, ImU32 color) {
    if (shape.tip == Tip::None || shape.points.size() < 2) return;
    std::optional<ImVec2> from = point(screen, shape.points[shape.points.size() - 2]);
    std::optional<ImVec2> to = point(screen, shape.points.back());
    if (!from || !to) return;

    if (shape.tip == Tip::Box) {
        list.AddRectFilled(ImVec2(to->x - BOX_HALF, to->y - BOX_HALF), ImVec2(to->x + BOX_HALF, to->y + BOX_HALF),
                           color);
        return;
    }

    const glm::vec2 delta(to->x - from->x, to->y - from->y);
    const float length = glm::length(delta);
    if (length <= 0.0f) return;
    const glm::vec2 along = delta / length;
    const glm::vec2 side(-along.y, along.x);
    const glm::vec2 end(to->x, to->y);
    const glm::vec2 apex = end + along * ARROW_LENGTH;
    const glm::vec2 left = end + side * ARROW_WIDTH;
    const glm::vec2 right = end - side * ARROW_WIDTH;
    list.AddTriangleFilled(ImVec2(apex.x, apex.y), ImVec2(left.x, left.y), ImVec2(right.x, right.y), color);
}

void collect(cinder::scene::Node& node, std::vector<Camera*>& cameras) {
    if (node.destroyed() || !node.isEnabled()) return;

    if (auto* camera = dynamic_cast<Camera*>(&node)) cameras.push_back(camera);
    for (cinder::scene::Node* child : node.children()) collect(*child, cameras);
}

}

std::vector<Camera*> sceneCameras(cinder::scene::Scene& scene) {
    std::vector<Camera*> cameras;
    for (cinder::scene::Node* root : scene.roots()) collect(*root, cameras);
    return cameras;
}

void drawFrustums(ImDrawList& list, const std::vector<Camera*>& cameras,
                  const glm::mat4& viewProjection, glm::vec2 origin, glm::vec2 size) {
    const Screen screen{viewProjection, origin, size};
    list.PushClipRect(ImVec2(origin.x, origin.y), ImVec2(origin.x + size.x, origin.y + size.y), true);
    for (Camera* camera : cameras) frustum(list, screen, *camera, EDGE, SIGHT);
    list.PopClipRect();
}

void drawColliders(ImDrawList& list, cinder::scene::Scene& scene, const glm::mat4& viewProjection,
                   glm::vec2 origin, glm::vec2 size) {
    const Screen screen{viewProjection, origin, size};
    list.PushClipRect(ImVec2(origin.x, origin.y), ImVec2(origin.x + size.x, origin.y + size.y), true);
    for (cinder::scene::Node* root : scene.roots()) colliders(list, screen, *root);
    list.PopClipRect();
}

void drawSelection(ImDrawList& list, cinder::scene::Node& node, const glm::mat4& viewProjection,
                   glm::vec2 origin, glm::vec2 size) {
    const Screen screen{viewProjection, origin, size};
    list.PushClipRect(ImVec2(origin.x, origin.y), ImVec2(origin.x + size.x, origin.y + size.y), true);

    if (auto* part = dynamic_cast<cinder::components::MeshPart*>(&node)) {
        box(list, screen, part->transform()->world(), SELECTED);
    } else if (auto* sprite = dynamic_cast<cinder::components::Sprite*>(&node)) {
        outline(list, screen, sprite->transform()->world(), sprite->size());
    } else if (auto* camera = dynamic_cast<Camera*>(&node)) {
        frustum(list, screen, *camera, SELECTED, SELECTED_SIGHT);
    } else if (auto* collider = dynamic_cast<cinder::physics::Collider*>(&node)) {
        shape(list, screen, collider->geometry(), SELECTED);
    }

    list.PopClipRect();
}

void drawManipulator(ImDrawList& list, const std::vector<HandleShape>& shapes, Handle hot,
                     const glm::mat4& viewProjection, glm::vec2 origin, glm::vec2 size) {
    const Screen screen{viewProjection, origin, size};
    list.PushClipRect(ImVec2(origin.x, origin.y), ImVec2(origin.x + size.x, origin.y + size.y), true);

    for (const bool back : {true, false}) {
        for (const HandleShape& shape : shapes) {
            if (shape.back != back) continue;
            const ImU32 color = shape.handle == hot ? HOT : colorOf(shape.handle);
            if (shape.filled) {
                fill(list, screen, shape, color);
                continue;
            }
            const ImU32 line = back ? withAlpha(color, BACK_ALPHA) : color;
            for (std::size_t i = 1; i < shape.points.size(); ++i) {
                segment(list, screen, shape.points[i - 1], shape.points[i], line, back ? THICKNESS : HANDLE_THICKNESS);
            }
            tip(list, screen, shape, color);
        }
    }

    list.PopClipRect();
}

}
