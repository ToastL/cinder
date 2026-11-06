#include "dev/GizmoLines.hpp"

#include "components/Camera.hpp"
#include "components/MeshPart.hpp"
#include "components/Sprite.hpp"
#include "gfx/pass/ViewCamera.hpp"
#include "physics/Collider.hpp"
#include "scene/Node.hpp"
#include "scene/Scene.hpp"
#include "scene/Transform.hpp"

#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/vec4.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace cinder::dev {

using cinder::components::Camera;

namespace {

constexpr glm::u8vec4 EDGE(235, 235, 235, 255);
constexpr glm::u8vec4 SIGHT(235, 235, 235, 110);
constexpr glm::u8vec4 SELECTED(255, 176, 46, 255);
constexpr glm::u8vec4 SELECTED_SIGHT(255, 176, 46, 110);
constexpr glm::u8vec4 AXIS_X(232, 72, 72, 255);
constexpr glm::u8vec4 AXIS_Y(120, 204, 80, 255);
constexpr glm::u8vec4 AXIS_Z(72, 128, 240, 255);
constexpr glm::u8vec4 NEUTRAL(235, 235, 235, 255);
constexpr glm::u8vec4 HOT(255, 226, 64, 255);
constexpr glm::u8vec4 COLLIDER(110, 220, 130, 200);
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

glm::vec2 project(const GizmoCanvas& canvas, const glm::vec4& position) {
    const glm::vec2 ndc = glm::vec2(position) / position.w;
    return canvas.origin + (ndc * 0.5f + 0.5f) * canvas.size;
}

glm::u8vec4 withAlpha(glm::u8vec4 color, int alpha) {
    color.a = static_cast<std::uint8_t>(alpha);
    return color;
}

void segment(GizmoStrokes& out, const GizmoCanvas& canvas, const glm::vec3& from, const glm::vec3& to,
             glm::u8vec4 color, float thickness = THICKNESS) {
    glm::vec4 a = canvas.viewProjection * glm::vec4(from, 1.0f);
    glm::vec4 b = canvas.viewProjection * glm::vec4(to, 1.0f);
    if (!clipSegment(a, b)) return;
    out.push_back(GizmoStroke{{project(canvas, a), project(canvas, b)}, color, thickness, false});
}

void frustum(GizmoStrokes& out, const GizmoCanvas& canvas, Camera& camera, glm::u8vec4 edge,
             glm::u8vec4 sight) {
    const cinder::scene::View seen = camera.view();
    cinder::gfx::pass::ViewCamera view;
    view.setView(seen);
    view.setViewSize(canvas.size.x, canvas.size.y);

    const std::array<glm::vec3, 8> corners = view.corners();
    const glm::vec3 eye(seen.world[3]);
    for (std::size_t i = 0; i < 4; ++i) {
        const std::size_t next = (i + 1) % 4;
        segment(out, canvas, corners[i], corners[next], edge);
        if (seen.orthographic) continue;

        segment(out, canvas, eye, corners[i], sight);
        segment(out, canvas, corners[i + 4], corners[next + 4], edge);
        segment(out, canvas, corners[i], corners[i + 4], edge);
    }
}

void box(GizmoStrokes& out, const GizmoCanvas& canvas, const glm::mat4& world, glm::u8vec4 color) {
    std::array<glm::vec3, 8> corners{};
    for (std::size_t i = 0; i < corners.size(); ++i) {
        const glm::vec3 local((i & 1) != 0 ? HALF_EXTENT : -HALF_EXTENT, (i & 2) != 0 ? HALF_EXTENT : -HALF_EXTENT,
                              (i & 4) != 0 ? HALF_EXTENT : -HALF_EXTENT);
        corners[i] = glm::vec3(world * glm::vec4(local, 1.0f));
    }
    for (std::size_t i = 0; i < corners.size(); ++i) {
        for (std::size_t bit = 1; bit < corners.size(); bit <<= 1) {
            if ((i & bit) == 0) segment(out, canvas, corners[i], corners[i | bit], color);
        }
    }
}

void outline(GizmoStrokes& out, const GizmoCanvas& canvas, const glm::mat4& world, glm::vec2 size) {
    const glm::vec2 half = size * 0.5f;
    std::array<glm::vec3, 4> corners{};
    for (std::size_t i = 0; i < corners.size(); ++i) {
        corners[i] = glm::vec3(world * glm::vec4(SIGNS[i] * half, 0.0f, 1.0f));
    }
    for (std::size_t i = 0; i < corners.size(); ++i) {
        segment(out, canvas, corners[i], corners[(i + 1) % corners.size()], SELECTED);
    }
}

void ball(GizmoStrokes& out, const GizmoCanvas& canvas, const glm::mat3& axes, const glm::vec3& centre,
          float radius, glm::u8vec4 color) {
    for (int axis = 0; axis < 3; ++axis) {
        const glm::vec3 u = axes[axis] * radius;
        const glm::vec3 v = axes[(axis + 1) % 3] * radius;
        glm::vec3 previous = centre + u;
        for (int i = 1; i <= CIRCLE_SEGMENTS; ++i) {
            const float angle = glm::two_pi<float>() * static_cast<float>(i) / CIRCLE_SEGMENTS;
            const glm::vec3 next = centre + u * std::cos(angle) + v * std::sin(angle);
            segment(out, canvas, previous, next, color);
            previous = next;
        }
    }
}

void shape(GizmoStrokes& out, const GizmoCanvas& canvas, const cinder::physics::Geometry& geometry,
           glm::u8vec4 color) {
    if (geometry.shape == cinder::physics::Shape::Box) {
        glm::mat4 world(1.0f);
        for (int i = 0; i < 3; ++i) world[i] = glm::vec4(geometry.axes[i] * (2.0f * geometry.halfExtents[i]), 0.0f);
        world[3] = glm::vec4(geometry.center, 1.0f);
        box(out, canvas, world, color);
        return;
    }

    const float half = cinder::physics::segmentHalf(geometry);
    const glm::vec3 along = cinder::physics::segmentAxis(geometry) * half;
    ball(out, canvas, geometry.axes, geometry.center - along, geometry.radius, color);
    if (half <= 0.0f) return;

    ball(out, canvas, geometry.axes, geometry.center + along, geometry.radius, color);
    for (const int axis : {0, 2}) {
        for (const float side : {-1.0f, 1.0f}) {
            const glm::vec3 offset = geometry.axes[axis] * (side * geometry.radius);
            segment(out, canvas, geometry.center - along + offset, geometry.center + along + offset, color);
        }
    }
}

void colliders(GizmoStrokes& out, const GizmoCanvas& canvas, cinder::scene::Node& node) {
    if (node.destroyed() || !node.isEnabled()) return;

    if (auto* collider = dynamic_cast<cinder::physics::Collider*>(&node)) {
        shape(out, canvas, collider->geometry(), COLLIDER);
    }
    for (cinder::scene::Node* child : node.children()) colliders(out, canvas, *child);
}

glm::u8vec4 colorOf(Handle handle) {
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

std::optional<glm::vec2> point(const GizmoCanvas& canvas, const glm::vec3& position) {
    const glm::vec4 clip = canvas.viewProjection * glm::vec4(position, 1.0f);
    if (clip.w <= 0.0f || clip.z < 0.0f) return std::nullopt;
    return project(canvas, clip);
}

void clockwise(std::vector<glm::vec2>& corners) {
    float area = 0.0f;
    for (std::size_t i = 0; i < corners.size(); ++i) {
        const glm::vec2& a = corners[i];
        const glm::vec2& b = corners[(i + 1) % corners.size()];
        area += a.x * b.y - b.x * a.y;
    }
    if (area < 0.0f) std::reverse(corners.begin(), corners.end());
}

void fill(GizmoStrokes& out, const GizmoCanvas& canvas, const HandleShape& shape, glm::u8vec4 color) {
    std::vector<glm::vec2> corners;
    for (const glm::vec3& position : shape.points) {
        std::optional<glm::vec2> corner = point(canvas, position);
        if (!corner) return;
        corners.push_back(*corner);
    }
    clockwise(corners);

    out.push_back(GizmoStroke{corners, withAlpha(color, FILL_ALPHA), 0.0f, true});
    out.push_back(GizmoStroke{std::move(corners), color, THICKNESS, true});
}

void tip(GizmoStrokes& out, const GizmoCanvas& canvas, const HandleShape& shape, glm::u8vec4 color) {
    if (shape.tip == Tip::None || shape.points.size() < 2) return;
    std::optional<glm::vec2> from = point(canvas, shape.points[shape.points.size() - 2]);
    std::optional<glm::vec2> to = point(canvas, shape.points.back());
    if (!from || !to) return;

    if (shape.tip == Tip::Box) {
        const glm::vec2 low = *to - BOX_HALF;
        const glm::vec2 high = *to + BOX_HALF;
        out.push_back(GizmoStroke{{low, {high.x, low.y}, high, {low.x, high.y}}, color, 0.0f, true});
        return;
    }

    const glm::vec2 delta = *to - *from;
    const float length = glm::length(delta);
    if (length <= 0.0f) return;
    const glm::vec2 along = delta / length;
    const glm::vec2 side(-along.y, along.x);
    std::vector<glm::vec2> corners{*to + along * ARROW_LENGTH, *to + side * ARROW_WIDTH, *to - side * ARROW_WIDTH};
    clockwise(corners);
    out.push_back(GizmoStroke{std::move(corners), color, 0.0f, true});
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

void frustumStrokes(GizmoStrokes& out, const std::vector<Camera*>& cameras, const GizmoCanvas& canvas) {
    for (Camera* camera : cameras) frustum(out, canvas, *camera, EDGE, SIGHT);
}

void colliderStrokes(GizmoStrokes& out, cinder::scene::Scene& scene, const GizmoCanvas& canvas) {
    for (cinder::scene::Node* root : scene.roots()) colliders(out, canvas, *root);
}

void selectionStrokes(GizmoStrokes& out, cinder::scene::Node& node, const GizmoCanvas& canvas) {
    if (auto* part = dynamic_cast<cinder::components::MeshPart*>(&node)) {
        box(out, canvas, part->transform()->world(), SELECTED);
    } else if (auto* sprite = dynamic_cast<cinder::components::Sprite*>(&node)) {
        outline(out, canvas, sprite->transform()->world(), sprite->size());
    } else if (auto* camera = dynamic_cast<Camera*>(&node)) {
        frustum(out, canvas, *camera, SELECTED, SELECTED_SIGHT);
    } else if (auto* collider = dynamic_cast<cinder::physics::Collider*>(&node)) {
        shape(out, canvas, collider->geometry(), SELECTED);
    }
}

void manipulatorStrokes(GizmoStrokes& out, const std::vector<HandleShape>& shapes, Handle hot,
                        const GizmoCanvas& canvas) {
    for (const bool back : {true, false}) {
        for (const HandleShape& shape : shapes) {
            if (shape.back != back) continue;
            const glm::u8vec4 color = shape.handle == hot ? HOT : colorOf(shape.handle);
            if (shape.filled) {
                fill(out, canvas, shape, color);
                continue;
            }
            const glm::u8vec4 line = back ? withAlpha(color, BACK_ALPHA) : color;
            for (std::size_t i = 1; i < shape.points.size(); ++i) {
                segment(out, canvas, shape.points[i - 1], shape.points[i], line, back ? THICKNESS : HANDLE_THICKNESS);
            }
            tip(out, canvas, shape, color);
        }
    }
}

}
