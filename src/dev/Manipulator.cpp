#include "dev/Manipulator.hpp"

#include "gfx/pass/ViewCamera.hpp"
#include "reflect/Reflect.hpp"
#include "scene/Node.hpp"
#include "scene/Scene.hpp"
#include "scene/Transform.hpp"

#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/matrix.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec4.hpp>

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstddef>
#include <string_view>
#include <utility>

namespace cinder::dev {
namespace {

using cinder::scene::Node;
using cinder::scene::Transform;

constexpr float PARALLEL = 1e-6f;
constexpr float DEGENERATE = 1e-10f;
constexpr float MIN_AXIS_POINTS = 15.0f;
constexpr float EDGE_ON = 0.26f;
constexpr float END_ON = 0.966f;
constexpr float PLANE_NEAR = 0.2f;
constexpr float PLANE_FAR = 0.45f;
constexpr float CENTRE_POINTS = 7.0f;
constexpr int RING_SEGMENTS = 64;
constexpr float FRONT_TOLERANCE = 0.05f;
constexpr float FULL_RING = 0.8f;
constexpr float FACING = 0.25f;
constexpr float RADIANS_PER_POINT = 0.01f;
constexpr float TANGENT_REACH = 0.1f;

const glm::vec3 UNIT_X(1.0f, 0.0f, 0.0f);
const glm::vec3 UNIT_Y(0.0f, 1.0f, 0.0f);
const glm::vec3 UNIT_Z(0.0f, 0.0f, 1.0f);

struct Plane {
    Handle handle;
    int first;
    int second;
};

constexpr Plane PLANES[] = {{Handle::XY, 0, 1}, {Handle::YZ, 1, 2}, {Handle::ZX, 2, 0}};
constexpr Handle AXES[] = {Handle::X, Handle::Y, Handle::Z};

int axisOf(Handle handle) {
    if (handle == Handle::Y) return 1;
    if (handle == Handle::Z) return 2;
    return 0;
}

const Plane* planeOf(Handle handle) {
    for (const Plane& plane : PLANES) {
        if (plane.handle == handle) return &plane;
    }
    return nullptr;
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

glm::vec3 unit(const glm::vec3& vector) {
    const float length = glm::length(vector);
    return length > DEGENERATE ? vector / length : glm::vec3(0.0f);
}

glm::vec3 perpendicular(const glm::vec3& normal) {
    return unit(glm::cross(normal, std::abs(normal.x) < 0.9f ? UNIT_X : UNIT_Y));
}

glm::vec3 ringPoint(const glm::vec3& centre, const glm::vec3& normal, float radius, float angle) {
    const glm::vec3 u = perpendicular(normal);
    const glm::vec3 v = glm::cross(normal, u);
    return centre + (u * std::cos(angle) + v * std::sin(angle)) * radius;
}

glm::mat4 parentWorld(Transform& transform) {
    Node* node = transform.node();
    Node* parent = node != nullptr ? node->parent() : nullptr;
    Transform* above = parent != nullptr ? parent->transform() : nullptr;
    return above != nullptr ? above->world() : glm::mat4(1.0f);
}

float snapped(float value, float step) { return std::round(value / step) * step; }

bool inFront(const glm::vec3& point, const Gizmo& gizmo, const glm::vec3& normal, const glm::vec3& toEye) {
    if (std::abs(glm::dot(normal, toEye)) >= FULL_RING) return true;
    return glm::dot(point - gizmo.pivot, toEye) >= -FRONT_TOLERANCE * gizmo.length;
}

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

std::optional<std::pair<glm::vec2, glm::vec2>> screenSegment(const GizmoView& view, const glm::vec3& from,
                                                             const glm::vec3& to) {
    glm::vec4 a = view.viewProjection * glm::vec4(from, 1.0f);
    glm::vec4 b = view.viewProjection * glm::vec4(to, 1.0f);
    if (a.z < 0.0f && b.z < 0.0f) return std::nullopt;
    if (a.z < 0.0f) a += (b - a) * (a.z / (a.z - b.z));
    else if (b.z < 0.0f) b += (a - b) * (b.z / (b.z - a.z));
    if (a.w <= PARALLEL || b.w <= PARALLEL) return std::nullopt;

    const glm::vec2 first = (glm::vec2(a) / a.w * 0.5f + 0.5f) * view.size;
    const glm::vec2 second = (glm::vec2(b) / b.w * 0.5f + 0.5f) * view.size;
    return std::make_pair(first, second);
}

float distanceToSegment(glm::vec2 point, glm::vec2 from, glm::vec2 to) {
    const glm::vec2 edge = to - from;
    const float squared = glm::dot(edge, edge);
    const float t = squared > DEGENERATE ? glm::clamp(glm::dot(point - from, edge) / squared, 0.0f, 1.0f) : 0.0f;
    return glm::distance(point, from + edge * t);
}

float cross2(glm::vec2 a, glm::vec2 b) { return a.x * b.y - a.y * b.x; }

float distanceTo(const HandleShape& shape, const GizmoView& view, glm::vec2 point) {
    float nearest = FLT_MAX;
    if (!shape.filled) {
        for (std::size_t i = 1; i < shape.points.size(); ++i) {
            if (auto segment = screenSegment(view, shape.points[i - 1], shape.points[i])) {
                nearest = std::min(nearest, distanceToSegment(point, segment->first, segment->second));
            }
        }
        return nearest;
    }

    std::vector<glm::vec2> corners;
    for (const glm::vec3& position : shape.points) {
        std::optional<glm::vec2> corner = toScreen(view, position);
        if (!corner) return FLT_MAX;
        corners.push_back(*corner);
    }

    bool positive = false;
    bool negative = false;
    for (std::size_t i = 0; i < corners.size(); ++i) {
        const glm::vec2 from = corners[i];
        const glm::vec2 to = corners[(i + 1) % corners.size()];
        const float side = cross2(to - from, point - from);
        positive = positive || side > 0.0f;
        negative = negative || side < 0.0f;
        nearest = std::min(nearest, distanceToSegment(point, from, to));
    }
    return positive && negative ? nearest : 0.0f;
}

bool longEnough(const GizmoView& view, const glm::vec3& from, const glm::vec3& to) {
    std::optional<glm::vec2> a = toScreen(view, from);
    std::optional<glm::vec2> b = toScreen(view, to);
    return a && b && glm::distance(*a, *b) >= MIN_AXIS_POINTS;
}

HandleShape centre(Handle handle, const Gizmo& gizmo, const GizmoView& view) {
    const float half = CENTRE_POINTS * gizmo.length / GIZMO_POINTS;
    const glm::vec3 right = unit(glm::vec3(view.camera[0])) * half;
    const glm::vec3 up = unit(glm::vec3(view.camera[1])) * half;
    HandleShape shape;
    shape.handle = handle;
    shape.filled = true;
    shape.points = {gizmo.pivot - right - up, gizmo.pivot + right - up, gizmo.pivot + right + up,
                    gizmo.pivot - right + up};
    return shape;
}

void axisHandles(std::vector<HandleShape>& out, const Gizmo& gizmo, const GizmoView& view, const glm::vec3& toEye,
                 Tip tip) {
    for (int i = 0; i < 3; ++i) {
        const glm::vec3 end = gizmo.pivot + gizmo.axes[i] * gizmo.length;
        if (std::abs(glm::dot(gizmo.axes[i], toEye)) > END_ON) continue;
        if (!longEnough(view, gizmo.pivot, end)) continue;
        HandleShape shape;
        shape.handle = AXES[i];
        shape.points = {gizmo.pivot, end};
        shape.tip = tip;
        out.push_back(std::move(shape));
    }
}

void planeHandles(std::vector<HandleShape>& out, const Gizmo& gizmo, const glm::vec3& toEye) {
    for (const Plane& plane : PLANES) {
        const glm::vec3 a = gizmo.axes[plane.first] * gizmo.length;
        const glm::vec3 b = gizmo.axes[plane.second] * gizmo.length;
        if (std::abs(glm::dot(unit(glm::cross(a, b)), toEye)) < EDGE_ON) continue;

        HandleShape shape;
        shape.handle = plane.handle;
        shape.filled = true;
        shape.points = {gizmo.pivot + a * PLANE_NEAR + b * PLANE_NEAR, gizmo.pivot + a * PLANE_FAR + b * PLANE_NEAR,
                        gizmo.pivot + a * PLANE_FAR + b * PLANE_FAR, gizmo.pivot + a * PLANE_NEAR + b * PLANE_FAR};
        out.push_back(std::move(shape));
    }
}

void ring(std::vector<HandleShape>& out, Handle handle, const Gizmo& gizmo, const glm::vec3& normal,
          const glm::vec3& toEye) {
    std::array<glm::vec3, RING_SEGMENTS + 1> points{};
    std::array<bool, RING_SEGMENTS + 1> front{};
    for (int k = 0; k <= RING_SEGMENTS; ++k) {
        const float angle = glm::two_pi<float>() * static_cast<float>(k % RING_SEGMENTS) / RING_SEGMENTS;
        points[k] = ringPoint(gizmo.pivot, normal, gizmo.length, angle);
        front[k] = inFront(points[k], gizmo, normal, toEye);
    }

    HandleShape run;
    run.handle = handle;
    run.back = !(front[0] && front[1]);
    run.points.push_back(points[0]);
    for (int k = 0; k < RING_SEGMENTS; ++k) {
        const bool back = !(front[k] && front[k + 1]);
        if (back != run.back) {
            out.push_back(std::move(run));
            run = HandleShape();
            run.handle = handle;
            run.back = back;
            run.points.push_back(points[k]);
        }
        run.points.push_back(points[k + 1]);
    }
    out.push_back(std::move(run));
}

}

GizmoView gizmoView(cinder::gfx::pass::ViewCamera& camera, glm::vec2 size) {
    return GizmoView{camera.viewProjection(), camera.view().world, camera.view().fovDegrees, size};
}

std::optional<glm::vec2> toScreen(const GizmoView& view, const glm::vec3& position) {
    const glm::vec4 clip = view.viewProjection * glm::vec4(position, 1.0f);
    if (clip.w <= PARALLEL || clip.z < 0.0f) return std::nullopt;
    return (glm::vec2(clip) / clip.w * 0.5f + 0.5f) * view.size;
}

std::optional<Gizmo> gizmoFor(Transform& transform, Tool tool, Space space, const GizmoView& view) {
    const glm::mat4 parent = parentWorld(transform);
    if (std::abs(glm::determinant(parent)) < DEGENERATE) return std::nullopt;

    Gizmo gizmo;
    gizmo.tool = tool;
    gizmo.pivot = transform.worldPosition();

    const glm::vec4 clip = view.viewProjection * glm::vec4(gizmo.pivot, 1.0f);
    if (clip.w <= PARALLEL || clip.z < 0.0f) return std::nullopt;
    gizmo.length = GIZMO_POINTS * 2.0f * clip.w * std::tan(glm::radians(view.fovDegrees) * 0.5f) / view.size.y;

    const glm::vec3& angles = transform.rotation();
    const glm::mat4 yaw = glm::rotate(parent, angles.y, UNIT_Y);
    const glm::mat4 pitch = glm::rotate(yaw, angles.x, UNIT_X);
    const glm::mat4 roll = glm::rotate(pitch, angles.z, UNIT_Z);

    if (tool == Tool::Rotate) {
        gizmo.axes = {unit(glm::vec3(yaw[0])), unit(glm::vec3(parent[1])), unit(glm::vec3(pitch[2]))};
    } else if (tool == Tool::Move && space == Space::World) {
        gizmo.axes = {UNIT_X, UNIT_Y, UNIT_Z};
    } else {
        gizmo.axes = {unit(glm::vec3(roll[0])), unit(glm::vec3(roll[1])), unit(glm::vec3(roll[2]))};
    }
    return gizmo;
}

std::vector<HandleShape> shapes(const Gizmo& gizmo, const GizmoView& view) {
    const glm::vec3 toEye = unit(glm::vec3(view.camera[3]) - gizmo.pivot);
    std::vector<HandleShape> out;
    switch (gizmo.tool) {
        case Tool::Move:
            out.push_back(centre(Handle::View, gizmo, view));
            planeHandles(out, gizmo, toEye);
            axisHandles(out, gizmo, view, toEye, Tip::Arrow);
            break;
        case Tool::Rotate:
            for (int i = 0; i < 3; ++i) ring(out, AXES[i], gizmo, gizmo.axes[i], toEye);
            break;
        case Tool::Scale:
            out.push_back(centre(Handle::Uniform, gizmo, view));
            axisHandles(out, gizmo, view, toEye, Tip::Box);
            break;
    }
    return out;
}

Handle hitHandle(const std::vector<HandleShape>& shapes, const GizmoView& view, glm::vec2 point) {
    Handle best = Handle::None;
    float nearest = HIT_POINTS;
    for (const HandleShape& shape : shapes) {
        if (shape.back) continue;
        const float distance = distanceTo(shape, view, point);
        if (distance < nearest) {
            nearest = distance;
            best = shape.handle;
        }
    }
    return best;
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
