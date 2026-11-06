#include "physics/Collide.hpp"

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/matrix.hpp>

#include <algorithm>
#include <cfloat>
#include <cmath>

namespace cinder::physics {

namespace {

constexpr float COINCIDENT = 1e-6f;
constexpr float PARALLEL = 1e-5f;
constexpr float EDGE_BIAS = 0.95f;
constexpr float FACE_BIAS = 0.98f;
constexpr int CLIP_FEATURE = 4;
constexpr int EDGE_FEATURE = 200;
constexpr int MAX_CLIPPED = 8;
constexpr float CLIP_SLACK = 1e-3f;
constexpr float TOUCH_SLACK = 1e-3f;
constexpr float LYING = 0.08f;
constexpr int SEGMENT_FEATURE = 100;

const glm::vec3 UP(0.0f, 1.0f, 0.0f);

const float CORNER_X[4] = {-1.0f, 1.0f, 1.0f, -1.0f};
const float CORNER_Y[4] = {-1.0f, -1.0f, 1.0f, 1.0f};

struct Polygon {
    std::array<glm::vec3, MAX_CLIPPED> points{};
    std::array<int, MAX_CLIPPED> features{};
    int count = 0;

    void add(const glm::vec3& point, int feature) {
        if (count >= MAX_CLIPPED) return;
        points[static_cast<std::size_t>(count)] = point;
        features[static_cast<std::size_t>(count)] = feature;
        ++count;
    }
};

bool sphereSphere(const Geometry& a, const Geometry& b, Manifold& out) {
    const glm::vec3 delta = b.center - a.center;
    const float reach = a.radius + b.radius;
    const float distanceSq = glm::dot(delta, delta);
    if (distanceSq > reach * reach) return false;

    const float distance = std::sqrt(distanceSq);
    const float depth = reach - distance;
    out.normal = distance > COINCIDENT ? delta / distance : UP;
    out.points[0] = {a.center + out.normal * (a.radius - depth * 0.5f), depth, 0};
    out.count = 1;
    return true;
}

bool sphereBox(const Geometry& sphere, const Geometry& box, Manifold& out) {
    const glm::vec3 local = glm::transpose(box.axes) * (sphere.center - box.center);
    const glm::vec3 closest = glm::clamp(local, -box.halfExtents, box.halfExtents);
    const glm::vec3 offset = local - closest;
    const float distanceSq = glm::dot(offset, offset);

    if (distanceSq > COINCIDENT * COINCIDENT) {
        if (distanceSq > sphere.radius * sphere.radius) return false;
        const float distance = std::sqrt(distanceSq);
        out.normal = -(box.axes * (offset / distance));
        out.points[0] = {box.center + box.axes * closest, sphere.radius - distance, 0};
        out.count = 1;
        return true;
    }

    int axis = 0;
    float gap = box.halfExtents[0] - std::abs(local[0]);
    for (int i = 1; i < 3; ++i) {
        const float candidate = box.halfExtents[i] - std::abs(local[i]);
        if (candidate < gap) {
            gap = candidate;
            axis = i;
        }
    }

    const float side = local[axis] < 0.0f ? -1.0f : 1.0f;
    glm::vec3 face = local;
    face[axis] = side * box.halfExtents[axis];
    out.normal = -box.axes[axis] * side;
    out.points[0] = {box.center + box.axes * face, sphere.radius + gap, 0};
    out.count = 1;
    return true;
}

glm::vec3 alongSegment(const Geometry& capsule, const glm::vec3& point) {
    const glm::vec3 axis = segmentAxis(capsule);
    const float half = segmentHalf(capsule);
    return capsule.center + axis * std::clamp(glm::dot(point - capsule.center, axis), -half, half);
}

Geometry ballAt(const glm::vec3& center, float radius) {
    Geometry out;
    out.shape = Shape::Sphere;
    out.center = center;
    out.radius = radius;
    out.halfExtents = glm::vec3(radius);
    return out;
}

void closestBetween(const Geometry& a, const Geometry& b, glm::vec3& onA, glm::vec3& onB) {
    const glm::vec3 axisA = segmentAxis(a);
    const glm::vec3 axisB = segmentAxis(b);
    const float halfA = segmentHalf(a);
    const float halfB = segmentHalf(b);

    const glm::vec3 between = b.center - a.center;
    const float cosine = glm::dot(axisA, axisB);
    const float denominator = 1.0f - cosine * cosine;
    const float alongA = glm::dot(between, axisA);
    const float alongB = glm::dot(between, axisB);

    float travelA = denominator > PARALLEL ? (alongA - cosine * alongB) / denominator : 0.0f;
    travelA = std::clamp(travelA, -halfA, halfA);
    const float travelB = std::clamp(cosine * travelA - alongB, -halfB, halfB);
    travelA = std::clamp(alongA + cosine * travelB, -halfA, halfA);

    onA = a.center + axisA * travelA;
    onB = b.center + axisB * travelB;
}

bool capsuleSphere(const Geometry& capsule, const Geometry& sphere, Manifold& out) {
    return sphereSphere(ballAt(alongSegment(capsule, sphere.center), capsule.radius), sphere, out);
}

bool capsuleCapsule(const Geometry& a, const Geometry& b, Manifold& out) {
    glm::vec3 onA(0.0f);
    glm::vec3 onB(0.0f);
    closestBetween(a, b, onA, onB);
    return sphereSphere(ballAt(onA, a.radius), ballAt(onB, b.radius), out);
}

float reachAlong(const Geometry& box, const glm::vec3& axis) {
    return box.halfExtents[0] * std::abs(glm::dot(axis, box.axes[0])) +
           box.halfExtents[1] * std::abs(glm::dot(axis, box.axes[1])) +
           box.halfExtents[2] * std::abs(glm::dot(axis, box.axes[2]));
}

bool apart(const Geometry& a, const Geometry& b, const glm::vec3& axis, const glm::vec3& between,
           float& depth) {
    depth = reachAlong(a, axis) + reachAlong(b, axis) - std::abs(glm::dot(between, axis));
    return depth < 0.0f;
}

Polygon clipTo(const Polygon& polygon, const glm::vec3& origin, const glm::vec3& axis, float limit,
               int plane) {
    const float bound = limit + CLIP_SLACK * (std::abs(limit) + 1.0f);
    Polygon out;
    for (int i = 0; i < polygon.count; ++i) {
        const std::size_t here = static_cast<std::size_t>(i);
        const std::size_t next = static_cast<std::size_t>((i + 1) % polygon.count);
        const glm::vec3& from = polygon.points[here];
        const glm::vec3& to = polygon.points[next];
        const float outside = glm::dot(from - origin, axis) - bound;
        const float beyond = glm::dot(to - origin, axis) - bound;

        if (outside <= 0.0f) out.add(from, polygon.features[here]);
        if (outside * beyond < 0.0f) {
            out.add(from + (to - from) * (outside / (outside - beyond)),
                    CLIP_FEATURE + plane * 4 + i);
        }
    }
    return out;
}

int incidentAxis(const Geometry& box, const glm::vec3& normal, float& side) {
    int axis = 0;
    float least = FLT_MAX;
    for (int i = 0; i < 3; ++i) {
        const float along = glm::dot(box.axes[i], normal);
        if (along < least) {
            least = along;
            axis = i;
            side = 1.0f;
        }
        if (-along < least) {
            least = -along;
            axis = i;
            side = -1.0f;
        }
    }
    return axis;
}

bool faceContact(const Geometry& a, const Geometry& b, const glm::vec3& normal, int axis,
                 Manifold& out) {
    const bool referenceIsA = axis < 3;
    const Geometry& reference = referenceIsA ? a : b;
    const Geometry& incident = referenceIsA ? b : a;
    const int referenceAxis = referenceIsA ? axis : axis - 3;
    const glm::vec3 outward = referenceIsA ? normal : -normal;
    const glm::vec3 face =
        reference.center + outward * reference.halfExtents[referenceAxis];

    float side = 1.0f;
    const int incidentIndex = incidentAxis(incident, outward, side);
    const glm::vec3 incidentNormal = incident.axes[incidentIndex] * side;
    const glm::vec3 incidentFace =
        incident.center + incidentNormal * incident.halfExtents[incidentIndex];
    const int u = (incidentIndex + 1) % 3;
    const int v = (incidentIndex + 2) % 3;

    Polygon polygon;
    for (int i = 0; i < 4; ++i) {
        polygon.add(incidentFace + incident.axes[u] * (CORNER_X[i] * incident.halfExtents[u]) +
                        incident.axes[v] * (CORNER_Y[i] * incident.halfExtents[v]),
                    i);
    }

    const int su = (referenceAxis + 1) % 3;
    const int sv = (referenceAxis + 2) % 3;
    const int sides[4] = {su, su, sv, sv};
    const float signs[4] = {1.0f, -1.0f, 1.0f, -1.0f};
    for (int plane = 0; plane < 4; ++plane) {
        const int index = sides[plane];
        polygon = clipTo(polygon, reference.center, reference.axes[index] * signs[plane],
                         reference.halfExtents[index], plane);
        if (polygon.count == 0) return false;
    }

    const int stamp = (referenceIsA ? 0 : 1 << 10) | (referenceAxis << 8);
    for (int i = 0; i < polygon.count; ++i) {
        const std::size_t index = static_cast<std::size_t>(i);
        const float separation = glm::dot(polygon.points[index] - face, outward);
        if (separation > TOUCH_SLACK) continue;

        const ContactPoint point{polygon.points[index], std::max(-separation, 0.0f),
                                 stamp | polygon.features[index]};
        if (out.count < Manifold::MAX_POINTS) {
            out.points[static_cast<std::size_t>(out.count)] = point;
            ++out.count;
            continue;
        }

        int shallowest = 0;
        for (int k = 1; k < Manifold::MAX_POINTS; ++k) {
            if (out.points[static_cast<std::size_t>(k)].depth <
                out.points[static_cast<std::size_t>(shallowest)].depth) {
                shallowest = k;
            }
        }
        if (point.depth > out.points[static_cast<std::size_t>(shallowest)].depth) {
            out.points[static_cast<std::size_t>(shallowest)] = point;
        }
    }

    if (out.count == 0) return false;
    out.normal = normal;
    return true;
}

glm::vec3 edgeCentre(const Geometry& box, const glm::vec3& direction, int along) {
    glm::vec3 point = box.center;
    for (int i = 0; i < 3; ++i) {
        if (i == along) continue;
        const float side = glm::dot(direction, box.axes[i]) >= 0.0f ? 1.0f : -1.0f;
        point += box.axes[i] * (side * box.halfExtents[i]);
    }
    return point;
}

bool edgeContact(const Geometry& a, const Geometry& b, const glm::vec3& normal, int axisA, int axisB,
                 float depth, Manifold& out) {
    const glm::vec3 centreA = edgeCentre(a, normal, axisA);
    const glm::vec3 centreB = edgeCentre(b, -normal, axisB);
    const glm::vec3 alongA = a.axes[axisA];
    const glm::vec3 alongB = b.axes[axisB];

    const glm::vec3 between = centreB - centreA;
    const float cosine = glm::dot(alongA, alongB);
    const float denominator = 1.0f - cosine * cosine;
    if (denominator < PARALLEL) return false;

    const float projectedA = glm::dot(between, alongA);
    const float projectedB = glm::dot(between, alongB);
    const float slideA = std::clamp((projectedA - cosine * projectedB) / denominator,
                                    -a.halfExtents[axisA], a.halfExtents[axisA]);
    const float slideB = std::clamp((cosine * projectedA - projectedB) / denominator,
                                    -b.halfExtents[axisB], b.halfExtents[axisB]);

    const glm::vec3 pointA = centreA + alongA * slideA;
    const glm::vec3 pointB = centreB + alongB * slideB;

    out.normal = normal;
    out.points[0] = {(pointA + pointB) * 0.5f, depth, EDGE_FEATURE + axisA * 3 + axisB};
    out.count = 1;
    return true;
}

glm::vec3 insideBox(const Geometry& box, const glm::vec3& point) {
    const glm::vec3 local = glm::transpose(box.axes) * (point - box.center);
    return box.center + box.axes * glm::clamp(local, -box.halfExtents, box.halfExtents);
}

bool capsuleBox(const Geometry& capsule, const Geometry& box, Manifold& out) {
    glm::vec3 onSegment = alongSegment(capsule, box.center);
    for (int i = 0; i < 4; ++i) onSegment = alongSegment(capsule, insideBox(box, onSegment));
    if (!sphereBox(ballAt(onSegment, capsule.radius), box, out)) return false;

    const glm::vec3 axis = segmentAxis(capsule);
    const float half = segmentHalf(capsule);
    if (half <= 0.0f || std::abs(glm::dot(axis, out.normal)) > LYING) return true;

    const glm::vec3 outward = -out.normal;
    int face = 0;
    for (int i = 1; i < 3; ++i) {
        if (std::abs(glm::dot(box.axes[i], outward)) > std::abs(glm::dot(box.axes[face], outward))) {
            face = i;
        }
    }

    const float side = glm::dot(box.axes[face], outward) < 0.0f ? -1.0f : 1.0f;
    const glm::vec3 plane = box.center + box.axes[face] * (side * box.halfExtents[face]);

    Manifold lying;
    lying.normal = out.normal;
    for (int end = 0; end < 2; ++end) {
        const glm::vec3 tip = capsule.center + axis * (end == 0 ? -half : half);
        const float gap = glm::dot(tip - plane, outward);
        if (gap > capsule.radius + TOUCH_SLACK) continue;

        const glm::vec3 local = glm::transpose(box.axes) * (tip - box.center);
        glm::vec3 clamped = glm::clamp(local, -box.halfExtents, box.halfExtents);
        clamped[face] = side * box.halfExtents[face];
        lying.points[static_cast<std::size_t>(lying.count)] = {box.center + box.axes * clamped,
                                                               std::max(capsule.radius - gap, 0.0f),
                                                               SEGMENT_FEATURE + end};
        ++lying.count;
    }

    if (lying.count == 2) out = lying;
    return true;
}

bool boxBox(const Geometry& a, const Geometry& b, Manifold& out) {
    const glm::vec3 between = b.center - a.center;

    float least = FLT_MAX;
    float chosen = FLT_MAX;
    int leastAxis = 0;
    glm::vec3 leastNormal(0.0f);
    for (int i = 0; i < 6; ++i) {
        const glm::vec3 axis = i < 3 ? a.axes[i] : b.axes[i - 3];
        float depth = 0.0f;
        if (apart(a, b, axis, between, depth)) return false;
        if (depth < chosen * (i < 3 ? 1.0f : FACE_BIAS)) {
            chosen = depth;
            leastAxis = i;
            leastNormal = axis;
        }
        least = std::min(least, depth);
    }

    float leastEdge = FLT_MAX;
    int edgeA = 0;
    int edgeB = 0;
    glm::vec3 edgeNormal(0.0f);
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            const glm::vec3 crossed = glm::cross(a.axes[i], b.axes[j]);
            const float length = glm::length(crossed);
            if (length < PARALLEL) continue;

            const glm::vec3 axis = crossed / length;
            float depth = 0.0f;
            if (apart(a, b, axis, between, depth)) return false;
            if (depth < leastEdge) {
                leastEdge = depth;
                edgeA = i;
                edgeB = j;
                edgeNormal = axis;
            }
        }
    }

    if (leastEdge < least * EDGE_BIAS) {
        const glm::vec3 normal = glm::dot(between, edgeNormal) < 0.0f ? -edgeNormal : edgeNormal;
        if (edgeContact(a, b, normal, edgeA, edgeB, leastEdge, out)) return true;
    }

    const glm::vec3 normal = glm::dot(between, leastNormal) < 0.0f ? -leastNormal : leastNormal;
    return faceContact(a, b, normal, leastAxis, out);
}

}

bool collide(const Geometry& a, const Geometry& b, Manifold& out) {
    out.count = 0;

    if (a.shape == Shape::Sphere && b.shape == Shape::Sphere) return sphereSphere(a, b, out);
    if (a.shape == Shape::Sphere && b.shape == Shape::Box) return sphereBox(a, b, out);
    if (a.shape == Shape::Capsule && b.shape == Shape::Sphere) return capsuleSphere(a, b, out);
    if (a.shape == Shape::Capsule && b.shape == Shape::Capsule) return capsuleCapsule(a, b, out);
    if (a.shape == Shape::Capsule && b.shape == Shape::Box) return capsuleBox(a, b, out);
    if (a.shape == Shape::Box && b.shape == Shape::Box) return boxBox(a, b, out);

    if (!collide(b, a, out)) return false;

    out.normal = -out.normal;
    return true;
}

}
