#include "dev/Manipulator.hpp"

#include "reflect/Reflect.hpp"
#include "scene/Node.hpp"
#include "scene/Scene.hpp"
#include "scene/Transform.hpp"

#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/matrix.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec4.hpp>

#include <cfloat>
#include <cmath>
#include <string_view>

namespace cinder::dev {
namespace {

using cinder::scene::Node;
using cinder::scene::Transform;
using namespace gizmo;

constexpr float FACING = 0.25f;
constexpr float RADIANS_PER_POINT = 0.01f;
constexpr float TANGENT_REACH = 0.1f;

int axisOf(Handle handle) {
    if (handle == Handle::Y) return 1;
    if (handle == Handle::Z) return 2;
    return 0;
}


bool supports(Tool tool, Handle handle) {
    const bool axis = handle == Handle::X || handle == Handle::Y || handle == Handle::Z;
    switch (tool) {
        case Tool::Move: return axis || planeOf(handle) != nullptr || handle == Handle::View;
        case Tool::Rotate: return axis;
        case Tool::Scale: return axis || handle == Handle::Uniform;
    }
    return false;
}


float snapped(float value, float step) { return std::round(value / step) * step; }

void writeProp(Transform& transform, std::string_view name, const glm::vec3& value) {
    for (const cinder::reflect::PropDef& def : cinder::reflect::props<Transform>()) {
        if (def.name() != name) continue;
        const float values[3] = {value.x, value.y, value.z};
        def.write(&transform, values);
    }
}


Transform* resolve(cinder::scene::Scene& scene, int id) {
    Node* node = scene.byId(id);
    return node != nullptr && !node->destroyed() ? node->transform() : nullptr;
}


}

bool Manipulation::begin(Node& node, const Gizmo& gizmo, Handle handle, const GizmoView& view, glm::vec2 point) {
    end();
    Transform* transform = node.transform();
    if (transform == nullptr || !supports(gizmo.tool, handle)) return false;

    const glm::mat4 parent = parentWorld(*transform);
    const float determinant = glm::determinant(parent);
    if (std::abs(determinant) < DEGENERATE) return false;

    tool_ = gizmo.tool;
    handle_ = handle;
    gizmo_ = gizmo;
    toParent_ = glm::inverse(parent);
    mirrored_ = determinant < 0.0f;
    position_ = transform->position();
    rotation_ = transform->rotation();
    scale_ = transform->scale();
    point_ = point;
    tangent_ = false;
    previous_ = 0.0f;
    angle_ = 0.0f;

    const Ray ray = rayThrough(view.viewProjection, point, view.size);
    bool ready = false;
    if (const Plane* plane = planeOf(handle)) {
        across_ = gizmo.axes[plane->first];
        up_ = gizmo.axes[plane->second];
        direction_ = unit(glm::cross(across_, up_));
    } else if (handle == Handle::View) {
        direction_ = unit(glm::vec3(view.camera[2]));
    } else {
        direction_ = gizmo.axes[axisOf(handle)];
    }

    if (handle == Handle::Uniform) {
        ready = true;
    } else if (tool_ == Tool::Rotate) {
        ready = beginTurn(view, ray, point);
    } else if (planeOf(handle) != nullptr || handle == Handle::View) {
        if (std::optional<glm::vec3> hit = onPlane(ray)) {
            hit_ = *hit;
            ready = true;
        }
    } else if (std::optional<float> t = along(ray)) {
        start_ = *t;
        ready = true;
    }

    if (ready) node_ = node.id();
    else end();
    return ready;
}

bool Manipulation::drag(cinder::scene::Scene& scene, const GizmoView& view, glm::vec2 point, bool snap) {
    Transform* transform = node_ ? resolve(scene, *node_) : nullptr;
    if (transform == nullptr) {
        end();
        return false;
    }

    if (point == point_) {
        if (tool_ == Tool::Move) writeProp(*transform, "position", position_);
        else if (tool_ == Tool::Rotate) writeProp(*transform, "rotation", rotation_);
        else writeProp(*transform, "scale", scale_);
        return true;
    }

    const Ray ray = rayThrough(view.viewProjection, point, view.size);
    glm::vec3 next(0.0f);
    std::string_view prop;

    if (tool_ == Tool::Move) {
        glm::vec3 travel(0.0f);
        if (planeOf(handle_) != nullptr || handle_ == Handle::View) {
            std::optional<glm::vec3> hit = onPlane(ray);
            if (!hit) return false;
            travel = *hit - hit_;
            if (snap && handle_ == Handle::View) {
                travel = glm::vec3(snapped(travel.x, MOVE_SNAP), snapped(travel.y, MOVE_SNAP),
                                   snapped(travel.z, MOVE_SNAP));
            } else if (snap) {
                const float aa = glm::dot(across_, across_);
                const float bb = glm::dot(up_, up_);
                const float ab = glm::dot(across_, up_);
                const float area = aa * bb - ab * ab;
                if (area > DEGENERATE) {
                    const float ta = glm::dot(travel, across_);
                    const float tb = glm::dot(travel, up_);
                    travel = across_ * snapped((ta * bb - tb * ab) / area, MOVE_SNAP)
                            + up_ * snapped((tb * aa - ta * ab) / area, MOVE_SNAP);
                }
            }
        } else {
            std::optional<float> t = along(ray);
            if (!t) return false;
            const float distance = snap ? snapped(*t - start_, MOVE_SNAP) : *t - start_;
            travel = direction_ * distance;
        }
        next = position_ + glm::vec3(toParent_ * glm::vec4(travel, 0.0f));
        prop = "position";
    } else if (tool_ == Tool::Rotate) {
        std::optional<float> angle = turn(view, point);
        if (!angle) return false;
        float value = mirrored_ ? -*angle : *angle;
        if (snap) value = snapped(value, glm::radians(ROTATE_SNAP_DEGREES));
        next = rotation_;
        next[axisOf(handle_)] += value;
        prop = "rotation";
    } else {
        float change = 0.0f;
        if (handle_ == Handle::Uniform) {
            const glm::vec2 moved = point - point_;
            change = (moved.x - moved.y) / GIZMO_POINTS;
        } else {
            std::optional<float> t = along(ray);
            if (!t) return false;
            change = (*t - start_) / gizmo_.length;
        }
        const float factor = 1.0f + (snap ? snapped(change, SCALE_SNAP) : change);
        next = scale_;
        if (handle_ == Handle::Uniform) next *= factor;
        else next[axisOf(handle_)] *= factor;
        prop = "scale";
    }

    writeProp(*transform, prop, next);
    return true;
}

bool Manipulation::cancel(cinder::scene::Scene& scene) {
    Transform* transform = node_ ? resolve(scene, *node_) : nullptr;
    end();
    if (transform == nullptr) return false;

    writeProp(*transform, "position", position_);
    writeProp(*transform, "rotation", rotation_);
    writeProp(*transform, "scale", scale_);
    return true;
}

void Manipulation::end() {
    node_.reset();
    handle_ = Handle::None;
}

std::optional<float> Manipulation::along(const Ray& ray) const {
    const glm::vec3 offset = gizmo_.pivot - ray.origin;
    const float facing = glm::dot(direction_, ray.direction);
    const float denominator = 1.0f - facing * facing;
    if (denominator < PARALLEL) return std::nullopt;
    return (facing * glm::dot(ray.direction, offset) - glm::dot(direction_, offset)) / denominator;
}

std::optional<glm::vec3> Manipulation::onPlane(const Ray& ray) const {
    const float facing = glm::dot(ray.direction, direction_);
    if (std::abs(facing) < PARALLEL) return std::nullopt;
    const float distance = glm::dot(gizmo_.pivot - ray.origin, direction_) / facing;
    if (distance < 0.0f) return std::nullopt;
    return ray.origin + ray.direction * distance;
}

bool Manipulation::beginTurn(const GizmoView& view, const Ray& ray, glm::vec2 point) {
    const glm::vec3 toEye = unit(glm::vec3(view.camera[3]) - gizmo_.pivot);
    const float minimum = gizmo_.length * 1e-3f;
    if (std::abs(glm::dot(direction_, toEye)) >= FACING) {
        std::optional<glm::vec3> hit = onPlane(ray);
        if (hit && glm::length(*hit - gizmo_.pivot) > minimum) {
            hit_ = *hit;
            return true;
        }
    }

    tangent_ = true;
    glm::vec3 grabbed = ringPoint(gizmo_.pivot, direction_, gizmo_.length, 0.0f);
    float nearest = FLT_MAX;
    for (int k = 0; k < RING_SEGMENTS; ++k) {
        const float angle = glm::two_pi<float>() * static_cast<float>(k) / RING_SEGMENTS;
        const glm::vec3 candidate = ringPoint(gizmo_.pivot, direction_, gizmo_.length, angle);
        if (!inFront(candidate, gizmo_, direction_, toEye)) continue;
        std::optional<glm::vec2> screen = toScreen(view, candidate);
        if (!screen || glm::distance(*screen, point) >= nearest) continue;
        nearest = glm::distance(*screen, point);
        grabbed = candidate;
    }

    const glm::vec3 tangent = unit(glm::cross(direction_, grabbed - gizmo_.pivot));
    std::optional<glm::vec2> from = toScreen(view, grabbed);
    std::optional<glm::vec2> to = toScreen(view, grabbed + tangent * gizmo_.length * TANGENT_REACH);
    if (!from || !to || glm::distance(*from, *to) < PARALLEL) return false;
    tangentScreen_ = glm::normalize(*to - *from);
    return true;
}

std::optional<float> Manipulation::turn(const GizmoView& view, glm::vec2 point) {
    if (tangent_) return glm::dot(point - point_, tangentScreen_) * RADIANS_PER_POINT;

    std::optional<glm::vec3> hit = onPlane(rayThrough(view.viewProjection, point, view.size));
    if (!hit) return std::nullopt;

    const glm::vec3 from = hit_ - gizmo_.pivot;
    const glm::vec3 to = *hit - gizmo_.pivot;
    const float current = std::atan2(glm::dot(glm::cross(from, to), direction_), glm::dot(from, to));
    float delta = current - previous_;
    if (delta > glm::pi<float>()) delta -= glm::two_pi<float>();
    else if (delta < -glm::pi<float>()) delta += glm::two_pi<float>();
    previous_ = current;
    angle_ += delta;
    return angle_;
}

}
