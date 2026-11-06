#include <doctest/doctest.h>

#include "components/Builtins.hpp"
#include "components/Camera.hpp"
#include "dev/GizmoLines.hpp"
#include "gfx/pass/ViewCamera.hpp"
#include "scene/Node.hpp"
#include "scene/NodeTypes.hpp"
#include "scene/Scene.hpp"
#include "scene/Transform.hpp"
#include "scene/View.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <cstddef>
#include <optional>
#include <string_view>

using cinder::components::Camera;
using cinder::dev::GizmoCanvas;
using cinder::dev::GizmoStroke;
using cinder::dev::GizmoStrokes;
using cinder::dev::Handle;
using cinder::dev::Space;
using cinder::dev::Tool;
using cinder::scene::Node;
using cinder::scene::NodeTypes;
using cinder::scene::Scene;

namespace {

const glm::vec2 ORIGIN(20.0f, 30.0f);
const glm::vec2 SIZE(400.0f, 300.0f);

struct World {
    NodeTypes types;
    Scene scene{types};
    cinder::gfx::pass::ViewCamera camera;

    World() {
        cinder::components::registerBuiltins(types);
        cinder::scene::View view;
        view.world = glm::inverse(glm::lookAt(glm::vec3(0.0f, 0.0f, 10.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
        view.fovDegrees = 60.0f;
        view.nearClip = 0.1f;
        view.farClip = 200.0f;
        camera.setView(view);
        camera.setViewSize(SIZE.x, SIZE.y);
    }

    GizmoCanvas canvas() { return GizmoCanvas{camera.viewProjection(), ORIGIN, SIZE}; }

    Node* add(std::string_view className, const glm::vec3& position) {
        Node* node = scene.create(className, nullptr);
        node->transform()->setPosition(position.x, position.y, position.z);
        return node;
    }
};

bool inside(const GizmoStroke& stroke) {
    for (const glm::vec2& point : stroke.points) {
        if (point.x < ORIGIN.x - 1e-3f || point.y < ORIGIN.y - 1e-3f) return false;
        if (point.x > ORIGIN.x + SIZE.x + 1e-3f || point.y > ORIGIN.y + SIZE.y + 1e-3f) return false;
    }
    return true;
}

float signedArea(const GizmoStroke& stroke) {
    float area = 0.0f;
    for (std::size_t i = 0; i < stroke.points.size(); ++i) {
        const glm::vec2& a = stroke.points[i];
        const glm::vec2& b = stroke.points[(i + 1) % stroke.points.size()];
        area += a.x * b.y - b.x * a.y;
    }
    return area;
}

}

TEST_CASE("a selected cube in view is its twelve edges, inside the panel") {
    World world;
    Node* part = world.add("MeshPart", glm::vec3(0.0f));

    GizmoStrokes strokes;
    cinder::dev::selectionStrokes(strokes, *part, world.canvas());

    REQUIRE(strokes.size() == 12);
    for (const GizmoStroke& stroke : strokes) {
        CHECK(stroke.points.size() == 2);
        CHECK_FALSE(stroke.filled());
        CHECK_FALSE(stroke.closed);
        CHECK(inside(stroke));
    }
}

TEST_CASE("a cube behind the view draws nothing, and one across the edge is cut at the panel") {
    World world;
    Node* behind = world.add("MeshPart", glm::vec3(0.0f, 0.0f, 20.0f));
    GizmoStrokes strokes;
    cinder::dev::selectionStrokes(strokes, *behind, world.canvas());
    CHECK(strokes.empty());

    Node* wide = world.add("MeshPart", glm::vec3(0.0f));
    wide->transform()->setScale(100.0f, 1.0f, 1.0f);
    cinder::dev::selectionStrokes(strokes, *wide, world.canvas());
    REQUIRE_FALSE(strokes.empty());
    for (const GizmoStroke& stroke : strokes) CHECK(inside(stroke));
}

TEST_CASE("a perspective camera is sixteen lines and an orthographic one is its near rectangle") {
    World world;
    Camera* camera = world.scene.create<Camera>(nullptr);
    camera->transform()->setPosition(0.0f, 0.0f, -5.0f);

    GizmoStrokes strokes;
    cinder::dev::frustumStrokes(strokes, {camera}, world.canvas());
    CHECK(strokes.size() == 16);

    camera->setProjection(Camera::Projection::Orthographic).setVirtualSize(4.0f, 3.0f);
    strokes.clear();
    cinder::dev::frustumStrokes(strokes, {camera}, world.canvas());
    CHECK(strokes.size() == 4);

    CHECK(cinder::dev::sceneCameras(world.scene).size() == 1);
    camera->setEnabled(false);
    CHECK(cinder::dev::sceneCameras(world.scene).empty());
}

TEST_CASE("the move gizmo fills its planes clockwise, outlines them, and tips its arrows") {
    World world;
    Node* node = world.add("Group", glm::vec3(0.0f));
    const cinder::dev::GizmoView view = cinder::dev::gizmoView(world.camera, SIZE);
    std::optional<cinder::dev::Gizmo> gizmo = cinder::dev::gizmoFor(*node->transform(), Tool::Move, Space::World, view);
    REQUIRE(gizmo);

    GizmoStrokes strokes;
    cinder::dev::manipulatorStrokes(strokes, cinder::dev::shapes(*gizmo, view), Handle::X, world.canvas());
    REQUIRE_FALSE(strokes.empty());

    int fills = 0;
    int triangles = 0;
    for (std::size_t i = 0; i < strokes.size(); ++i) {
        const GizmoStroke& stroke = strokes[i];
        if (!stroke.filled()) continue;
        ++fills;
        CHECK(signedArea(stroke) >= 0.0f);
        if (stroke.points.size() == 3) ++triangles;
        if (stroke.points.size() == 4 && i + 1 < strokes.size() && strokes[i + 1].closed) {
            CHECK(strokes[i + 1].points == stroke.points);
        }
    }
    CHECK(fills > 0);
    CHECK(triangles == 2);
}
