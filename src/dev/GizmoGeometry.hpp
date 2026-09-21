#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <array>
#include <optional>
#include <vector>

namespace cinder::gfx::pass { class ViewCamera; }
namespace cinder::scene { class Transform; }

namespace cinder::dev {

enum class Tool { Move, Rotate, Scale };
enum class Space { World, Local };
enum class Handle { None, X, Y, Z, XY, YZ, ZX, View, Uniform };
enum class Tip { None, Arrow, Box };

constexpr float GIZMO_POINTS = 100.0f;
constexpr float HIT_POINTS = 8.0f;

struct GizmoView {
    glm::mat4 viewProjection{1.0f};
    glm::mat4 camera{1.0f};
    float fovDegrees = 60.0f;
    glm::vec2 size{1.0f};
};

struct Gizmo {
    Tool tool = Tool::Move;
    glm::vec3 pivot{0.0f};
    std::array<glm::vec3, 3> axes{};
    float length = 1.0f;
};

struct HandleShape {
    Handle handle = Handle::None;
    std::vector<glm::vec3> points;
    bool filled = false;
    bool back = false;
    Tip tip = Tip::None;
};

GizmoView gizmoView(cinder::gfx::pass::ViewCamera& camera, glm::vec2 size);
std::optional<glm::vec2> toScreen(const GizmoView& view, const glm::vec3& position);

std::optional<Gizmo> gizmoFor(cinder::scene::Transform& transform, Tool tool, Space space, const GizmoView& view);
std::vector<HandleShape> shapes(const Gizmo& gizmo, const GizmoView& view);
Handle hitHandle(const std::vector<HandleShape>& shapes, const GizmoView& view, glm::vec2 point);

namespace gizmo {

constexpr float PARALLEL = 1e-6f;
constexpr float DEGENERATE = 1e-10f;
constexpr int RING_SEGMENTS = 64;

struct Plane {
    Handle handle;
    int first;
    int second;
};

const Plane* planeOf(Handle handle);
glm::vec3 unit(const glm::vec3& vector);
glm::vec3 ringPoint(const glm::vec3& centre, const glm::vec3& normal, float radius, float angle);
glm::mat4 parentWorld(cinder::scene::Transform& transform);
bool inFront(const glm::vec3& point, const Gizmo& gizmo, const glm::vec3& normal, const glm::vec3& toEye);

}

}
