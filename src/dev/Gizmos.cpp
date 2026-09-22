#include "dev/Gizmos.hpp"

#include <imgui.h>

#include <vector>

namespace cinder::dev {

using cinder::components::Camera;

namespace {

ImU32 packed(glm::u8vec4 color) {
    return IM_COL32(color.r, color.g, color.b, color.a);
}

void emit(ImDrawList& list, const GizmoStrokes& strokes, glm::vec2 origin, glm::vec2 size) {
    list.PushClipRect(ImVec2(origin.x, origin.y), ImVec2(origin.x + size.x, origin.y + size.y), true);
    std::vector<ImVec2> points;
    for (const GizmoStroke& stroke : strokes) {
        points.clear();
        for (const glm::vec2& point : stroke.points) points.emplace_back(point.x, point.y);
        const int count = static_cast<int>(points.size());
        if (stroke.filled()) {
            list.AddConvexPolyFilled(points.data(), count, packed(stroke.color));
        } else {
            list.AddPolyline(points.data(), count, packed(stroke.color),
                             stroke.closed ? ImDrawFlags_Closed : ImDrawFlags_None, stroke.thickness);
        }
    }
    list.PopClipRect();
}

}

void drawFrustums(ImDrawList& list, const std::vector<Camera*>& cameras,
                  const glm::mat4& viewProjection, glm::vec2 origin, glm::vec2 size) {
    GizmoStrokes strokes;
    frustumStrokes(strokes, cameras, GizmoCanvas{viewProjection, origin, size});
    emit(list, strokes, origin, size);
}

void drawColliders(ImDrawList& list, cinder::scene::Scene& scene, const glm::mat4& viewProjection,
                   glm::vec2 origin, glm::vec2 size) {
    GizmoStrokes strokes;
    colliderStrokes(strokes, scene, GizmoCanvas{viewProjection, origin, size});
    emit(list, strokes, origin, size);
}

void drawSelection(ImDrawList& list, cinder::scene::Node& node, const glm::mat4& viewProjection,
                   glm::vec2 origin, glm::vec2 size) {
    GizmoStrokes strokes;
    selectionStrokes(strokes, node, GizmoCanvas{viewProjection, origin, size});
    emit(list, strokes, origin, size);
}

void drawManipulator(ImDrawList& list, const std::vector<HandleShape>& shapes, Handle hot,
                     const glm::mat4& viewProjection, glm::vec2 origin, glm::vec2 size) {
    GizmoStrokes strokes;
    manipulatorStrokes(strokes, shapes, hot, GizmoCanvas{viewProjection, origin, size});
    emit(list, strokes, origin, size);
}

}
