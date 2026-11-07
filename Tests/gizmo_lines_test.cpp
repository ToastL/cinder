#include <doctest/doctest.h>

#include "Dev/GizmoLines.hpp"
#include "Gfx/Pass/ViewCamera.hpp"
#include "Components/Actors.hpp"
#include "Components/Builtins.hpp"
#include "Scene/Actor.hpp"
#include "Scene/ClassRegistry.hpp"
#include "Scene/SceneComponent.hpp"
#include "Scene/Scene.hpp"
#include "Scene/Transform.hpp"
#include "Scene/View.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <cstddef>
#include <optional>
#include <string_view>

using Camera = cinder::Components::CameraComponent;
using cinder::Dev::GizmoCanvas;
using cinder::Dev::GizmoStroke;
using cinder::Dev::GizmoStrokes;
using cinder::Dev::Handle;
using cinder::Dev::Space;
using cinder::Dev::Tool;
using cinder::Scene::Actor;
using cinder::Scene::ClassRegistry;
using cinder::Scene::Scene;

namespace {

const glm::vec2 ORIGIN(20.0f, 30.0f);
const glm::vec2 SIZE(400.0f, 300.0f);

struct World {
    ClassRegistry types;
    Scene scene{types};
    cinder::Gfx::Pass::ViewCamera camera;

    World() {
        cinder::Components::registerBuiltins(types);
        cinder::Scene::View view;
        view.world = glm::inverse(glm::lookAt(glm::vec3(0.0f, 0.0f, 10.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
        view.fovDegrees = 60.0f;
        view.nearClip = 0.1f;
        view.farClip = 200.0f;
        camera.setView(view);
        camera.setViewSize(SIZE.x, SIZE.y);
    }

    GizmoCanvas canvas() { return GizmoCanvas{camera.viewProjection(), ORIGIN, SIZE}; }

    Actor* add(std::string_view className, const glm::vec3& position) {
        Actor* actor = scene.spawn(className, nullptr);
        if (actor->rootComponent() == nullptr) actor->addComponent<cinder::Scene::SceneComponent>();
        actor->transform()->setPosition(position.x, position.y, position.z);
        return actor;
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
    Actor* part = world.add("StaticMeshActor", glm::vec3(0.0f));

    GizmoStrokes strokes;
    cinder::Dev::selectionStrokes(strokes, *part, world.canvas());

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
    Actor* behind = world.add("StaticMeshActor", glm::vec3(0.0f, 0.0f, 20.0f));
    GizmoStrokes strokes;
    cinder::Dev::selectionStrokes(strokes, *behind, world.canvas());
    CHECK(strokes.empty());

    Actor* wide = world.add("StaticMeshActor", glm::vec3(0.0f));
    wide->transform()->setScale(100.0f, 1.0f, 1.0f);
    cinder::Dev::selectionStrokes(strokes, *wide, world.canvas());
    REQUIRE_FALSE(strokes.empty());
    for (const GizmoStroke& stroke : strokes) CHECK(inside(stroke));
}

TEST_CASE("a perspective camera is sixteen lines and an orthographic one is its near rectangle") {
    World world;
    Camera* camera = &world.scene.spawn<cinder::Components::CameraActor>()->camera();
    camera->transform().setPosition(0.0f, 0.0f, -5.0f);

    GizmoStrokes strokes;
    cinder::Dev::frustumStrokes(strokes, {camera}, world.canvas());
    CHECK(strokes.size() == 16);

    camera->setProjection(Camera::Projection::Orthographic).setVirtualSize(4.0f, 3.0f);
    strokes.clear();
    cinder::Dev::frustumStrokes(strokes, {camera}, world.canvas());
    CHECK(strokes.size() == 4);

    CHECK(cinder::Dev::sceneCameras(world.scene).size() == 1);
    camera->setEnabled(false);
    CHECK(cinder::Dev::sceneCameras(world.scene).empty());
}

TEST_CASE("the move gizmo fills its planes clockwise, outlines them, and tips its arrows") {
    World world;
    Actor* node = world.add("Actor", glm::vec3(0.0f));
    const cinder::Dev::GizmoView view = cinder::Dev::gizmoView(world.camera, SIZE);
    std::optional<cinder::Dev::Gizmo> gizmo = cinder::Dev::gizmoFor(*node->transform(), Tool::Move, Space::World, view);
    REQUIRE(gizmo);

    GizmoStrokes strokes;
    cinder::Dev::manipulatorStrokes(strokes, cinder::Dev::shapes(*gizmo, view), Handle::X, world.canvas());
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
