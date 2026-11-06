#pragma once

#include "dev/Picking.hpp"
#include "dev/GizmoGeometry.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <optional>

namespace cinder::scene {
class Node;
class Scene;
class Transform;
}

namespace cinder::dev {

constexpr float MOVE_SNAP = 1.0f;

constexpr float ROTATE_SNAP_DEGREES = 15.0f;

constexpr float SCALE_SNAP = 0.1f;

class Manipulation {
public:
    bool active() const { return node_.has_value(); }
    Tool tool() const { return tool_; }
    Handle handle() const { return handle_; }
    std::optional<int> node() const { return node_; }

    bool begin(cinder::scene::Node& node, const Gizmo& gizmo, Handle handle, const GizmoView& view,
               glm::vec2 point);
    bool drag(cinder::scene::Scene& scene, const GizmoView& view, glm::vec2 point, bool snap);
    bool cancel(cinder::scene::Scene& scene);
    void end();

private:
    std::optional<float> along(const Ray& ray) const;
    std::optional<glm::vec3> onPlane(const Ray& ray) const;
    std::optional<float> turn(const GizmoView& view, glm::vec2 point);
    bool beginTurn(const GizmoView& view, const Ray& ray, glm::vec2 point);

    std::optional<int> node_;
    Tool tool_ = Tool::Move;
    Handle handle_ = Handle::None;
    Gizmo gizmo_;
    glm::vec3 direction_{0.0f};
    glm::vec3 across_{0.0f};
    glm::vec3 up_{0.0f};
    glm::mat4 toParent_{1.0f};
    bool mirrored_ = false;

    glm::vec3 position_{0.0f};
    glm::vec3 rotation_{0.0f};
    glm::vec3 scale_{1.0f};

    glm::vec2 point_{0.0f};
    float start_ = 0.0f;
    glm::vec3 hit_{0.0f};
    bool tangent_ = false;
    glm::vec2 tangentScreen_{0.0f};
    float previous_ = 0.0f;
    float angle_ = 0.0f;
};

}
