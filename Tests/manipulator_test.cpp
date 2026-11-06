#include <doctest/doctest.h>

#include "components/Builtins.hpp"
#include "dev/History.hpp"
#include "dev/Manipulator.hpp"
#include "gfx/pass/ViewCamera.hpp"
#include "scene/Node.hpp"
#include "scene/NodeTypes.hpp"
#include "scene/Scene.hpp"
#include "scene/Transform.hpp"
#include "scene/View.hpp"

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat3x3.hpp>
#include <glm/trigonometric.hpp>

#include <cmath>
#include <cstddef>
#include <set>
#include <string_view>

using cinder::dev::Gizmo;
using cinder::dev::GizmoView;
using cinder::dev::Handle;
using cinder::dev::History;
using cinder::dev::Manipulation;
using cinder::dev::Space;
using cinder::dev::Tool;
using cinder::dev::gizmoFor;
using cinder::dev::hitHandle;
using cinder::dev::shapes;
using cinder::scene::Node;
using cinder::scene::NodeTypes;
using cinder::scene::Scene;
using cinder::scene::Transform;

namespace {

const glm::vec2 SIZE(400.0f, 300.0f);
const glm::vec3 UP(0.0f, 1.0f, 0.0f);

struct World {
    NodeTypes types;
    Scene scene{types};
    cinder::gfx::pass::ViewCamera camera;

    World() {
        cinder::components::registerBuiltins(types);
        lookFrom(glm::vec3(0.0f, 0.0f, 10.0f), glm::vec3(0.0f));
    }

    void lookFrom(const glm::vec3& eye, const glm::vec3& target) {
        const glm::vec3 forward = glm::normalize(target - eye);
        const glm::vec3 up = std::abs(forward.y) < 0.9f ? UP : glm::vec3(0.0f, 0.0f, 1.0f);
        cinder::scene::View view;
        view.world = glm::inverse(glm::lookAt(eye, target, up));
        view.fovDegrees = 60.0f;
        view.nearClip = 0.1f;
        view.farClip = 200.0f;
        camera.setView(view);
        camera.setViewSize(SIZE.x, SIZE.y);
    }

    GizmoView view() { return cinder::dev::gizmoView(camera, SIZE); }

    glm::vec2 screen(const glm::vec3& position) {
        std::optional<glm::vec2> point = cinder::dev::toScreen(view(), position);
        REQUIRE(point);
        return *point;
    }

    Node* add(std::string_view className, const glm::vec3& position, Node* parent = nullptr) {
        Node* node = scene.create(className, parent);
        if (Transform* transform = node->transform()) transform->setPosition(position.x, position.y, position.z);
        return node;
    }

    Gizmo gizmo(Node& node, Tool tool, Space space = Space::World) {
        std::optional<Gizmo> found = gizmoFor(*node.transform(), tool, space, view());
        REQUIRE(found);
        return *found;
    }

    Handle hit(const Gizmo& gizmo, const glm::vec3& position) {
        return hitHandle(shapes(gizmo, view()), view(), screen(position));
    }

    bool grab(Manipulation& drag, Node& node, const Gizmo& gizmo, Handle handle, const glm::vec3& at) {
        return drag.begin(node, gizmo, handle, view(), screen(at));
    }

    bool to(Manipulation& drag, const glm::vec3& position, bool snap = false) {
        return drag.drag(scene, view(), screen(position), snap);
    }
};

glm::vec3 perpendicular(const glm::vec3& normal) {
    return glm::normalize(glm::cross(normal, std::abs(normal.x) < 0.9f ? glm::vec3(1.0f, 0.0f, 0.0f) : UP));
}

glm::vec3 onRing(const Gizmo& gizmo, const glm::vec3& normal, float degrees) {
    const glm::vec3 u = perpendicular(normal);
    const float radians = glm::radians(degrees);
    return gizmo.pivot + (u * std::cos(radians) + glm::cross(normal, u) * std::sin(radians)) * gizmo.length;
}

std::set<Handle> handlesOf(const Gizmo& gizmo, const GizmoView& view) {
    std::set<Handle> found;
    for (const cinder::dev::HandleShape& shape : shapes(gizmo, view)) found.insert(shape.handle);
    return found;
}

}

TEST_CASE("a world X drag moves only position.x by the mouse's travel along X") {
    World world;
    Node* box = world.add("MeshPart", glm::vec3(0.0f, 1.0f, 0.0f));
    const Gizmo gizmo = world.gizmo(*box, Tool::Move);
    const glm::vec3 grab = gizmo.pivot + glm::vec3(gizmo.length * 0.8f, 0.0f, 0.0f);
    CHECK(world.hit(gizmo, grab) == Handle::X);

    Manipulation drag;
    REQUIRE(world.grab(drag, *box, gizmo, Handle::X, grab));
    CHECK(drag.active());
    REQUIRE(world.to(drag, grab + glm::vec3(2.5f, 0.0f, 0.0f)));

    const glm::vec3& position = box->transform()->position();
    CHECK(position.x == doctest::Approx(2.5f).epsilon(1e-3));
    CHECK(position.y == 1.0f);
    CHECK(position.z == 0.0f);
}

TEST_CASE("a drag under a rotated, scaled Group lands the node where the ray says") {
    World world;
    Node* group = world.add("Group", glm::vec3(0.0f));
    group->transform()->setRotation(0.0f, glm::radians(90.0f), 0.0f);
    group->transform()->setScale(2.0f);
    Node* box = world.add("MeshPart", glm::vec3(1.0f, 0.0f, 0.0f), group);
    REQUIRE(box->transform()->worldPosition().z == doctest::Approx(-2.0f));

    const Gizmo gizmo = world.gizmo(*box, Tool::Move);
    const glm::vec3 grab = gizmo.pivot + glm::vec3(gizmo.length * 0.8f, 0.0f, 0.0f);
    Manipulation drag;
    REQUIRE(world.grab(drag, *box, gizmo, Handle::X, grab));
    REQUIRE(world.to(drag, grab + glm::vec3(3.0f, 0.0f, 0.0f)));

    const glm::vec3 placed = box->transform()->worldPosition();
    CHECK(placed.x == doctest::Approx(3.0f).epsilon(1e-3));
    CHECK(placed.y == doctest::Approx(0.0f));
    CHECK(placed.z == doctest::Approx(-2.0f));
}

TEST_CASE("a drag under a Folder writes world coordinates") {
    World world;
    Node* folder = world.scene.create("Folder", nullptr);
    Node* sprite = world.add("Sprite", glm::vec3(1.0f, 2.0f, 0.0f), folder);

    const Gizmo gizmo = world.gizmo(*sprite, Tool::Move);
    const glm::vec3 grab = gizmo.pivot + glm::vec3(gizmo.length * 0.8f, 0.0f, 0.0f);
    Manipulation drag;
    REQUIRE(world.grab(drag, *sprite, gizmo, Handle::X, grab));
    REQUIRE(world.to(drag, grab + glm::vec3(2.0f, 0.0f, 0.0f)));

    CHECK(sprite->transform()->position().x == doctest::Approx(3.0f).epsilon(1e-3));
    CHECK(sprite->transform()->position().y == 2.0f);
}

TEST_CASE("a local drag follows the node's own axes") {
    World world;
    Node* box = world.add("MeshPart", glm::vec3(0.0f));
    box->transform()->setRotation(0.0f, 0.0f, glm::radians(90.0f));

    const Gizmo gizmo = world.gizmo(*box, Tool::Move, Space::Local);
    CHECK(gizmo.axes[0].y == doctest::Approx(1.0f));

    const glm::vec3 grab = gizmo.pivot + gizmo.axes[0] * gizmo.length * 0.8f;
    CHECK(world.hit(gizmo, grab) == Handle::X);
    Manipulation drag;
    REQUIRE(world.grab(drag, *box, gizmo, Handle::X, grab));
    REQUIRE(world.to(drag, grab + glm::vec3(0.0f, 2.0f, 0.0f)));

    CHECK(box->transform()->position().x == doctest::Approx(0.0f));
    CHECK(box->transform()->position().y == doctest::Approx(2.0f).epsilon(1e-3));
}

TEST_CASE("an axis pointing at the camera and planes seen edge-on are not offered") {
    World world;
    Node* sprite = world.add("Sprite", glm::vec3(0.0f));
    const Gizmo gizmo = world.gizmo(*sprite, Tool::Move);

    const std::set<Handle> offered = handlesOf(gizmo, world.view());
    CHECK(offered == std::set<Handle>{Handle::View, Handle::XY, Handle::X, Handle::Y});
    CHECK(world.hit(gizmo, gizmo.pivot) == Handle::View);
    CHECK(world.hit(gizmo, gizmo.pivot + glm::vec3(gizmo.length * 3.0f, 0.0f, 0.0f)) == Handle::None);
}

TEST_CASE("a plane drag and a view drag follow the ray's hit exactly") {
    World world;
    Node* sprite = world.add("Sprite", glm::vec3(0.0f));
    const Gizmo gizmo = world.gizmo(*sprite, Tool::Move);
    const glm::vec3 grab = gizmo.pivot + glm::vec3(gizmo.length * 0.3f, gizmo.length * 0.3f, 0.0f);
    CHECK(world.hit(gizmo, grab) == Handle::XY);

    Manipulation drag;
    REQUIRE(world.grab(drag, *sprite, gizmo, Handle::XY, grab));
    REQUIRE(world.to(drag, grab + glm::vec3(1.5f, -0.5f, 0.0f)));
    CHECK(sprite->transform()->position().x == doctest::Approx(1.5f).epsilon(1e-3));
    CHECK(sprite->transform()->position().y == doctest::Approx(-0.5f).epsilon(1e-3));
    CHECK(sprite->transform()->position().z == doctest::Approx(0.0f));
    drag.end();

    const Gizmo moved = world.gizmo(*sprite, Tool::Move);
    REQUIRE(world.grab(drag, *sprite, moved, Handle::View, moved.pivot));
    REQUIRE(world.to(drag, glm::vec3(-2.0f, 1.0f, 0.0f)));
    CHECK(sprite->transform()->position().x == doctest::Approx(-2.0f).epsilon(1e-3));
    CHECK(sprite->transform()->position().y == doctest::Approx(1.0f).epsilon(1e-3));
}

TEST_CASE("each ring turns one Euler angle, about that ring's axis") {
    World world;
    Node* group = world.add("Group", glm::vec3(0.0f));
    group->transform()->setRotation(0.2f, 0.4f, 0.1f);
    group->transform()->setScale(1.5f);
    Node* box = world.add("MeshPart", glm::vec3(0.0f), group);
    const glm::vec3 start(0.3f, 0.5f, 0.2f);

    for (int i = 0; i < 3; ++i) {
        CAPTURE(i);
        box->transform()->setRotation(start.x, start.y, start.z);
        const glm::mat3 before(box->transform()->world());

        const glm::vec3 normal = world.gizmo(*box, Tool::Rotate).axes[static_cast<std::size_t>(i)];
        world.lookFrom(normal * 10.0f, glm::vec3(0.0f));
        const Gizmo gizmo = world.gizmo(*box, Tool::Rotate);
        const Handle handle = i == 0 ? Handle::X : i == 1 ? Handle::Y : Handle::Z;

        Manipulation drag;
        REQUIRE(world.grab(drag, *box, gizmo, handle, onRing(gizmo, normal, 0.0f)));
        REQUIRE(world.to(drag, onRing(gizmo, normal, 90.0f)));

        const glm::vec3& rotation = box->transform()->rotation();
        for (int j = 0; j < 3; ++j) {
            if (j == i) CHECK(rotation[j] == doctest::Approx(start[j] + glm::radians(90.0f)).epsilon(1e-3));
            else CHECK(rotation[j] == start[j]);
        }

        const glm::mat3 expected = glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), normal)) * before;
        const glm::mat3 after(box->transform()->world());
        for (int column = 0; column < 3; ++column) {
            for (int row = 0; row < 3; ++row) {
                CHECK(after[column][row] == doctest::Approx(expected[column][row]).epsilon(1e-3));
            }
        }
    }
}

TEST_CASE("a ring facing the camera from off-centre can be grabbed all the way round") {
    World world;
    Node* box = world.add("MeshPart", glm::vec3(2.5f, 0.0f, 0.0f));
    const Gizmo gizmo = world.gizmo(*box, Tool::Rotate);
    const glm::vec3 normal(0.0f, 0.0f, 1.0f);

    for (float degrees : {45.0f, 100.0f, 135.0f, 225.0f, 315.0f}) {
        CAPTURE(degrees);
        const glm::vec3 u(1.0f, 0.0f, 0.0f);
        const float radians = glm::radians(degrees);
        const glm::vec3 point = gizmo.pivot + (u * std::cos(radians) + glm::cross(normal, u) * std::sin(radians)) * gizmo.length;
        CHECK(world.hit(gizmo, point) == Handle::Z);
    }
}

TEST_CASE("an edge-on ring turns with the mouse along its tangent") {
    World world;
    Node* box = world.add("MeshPart", glm::vec3(0.0f));
    const Gizmo gizmo = world.gizmo(*box, Tool::Rotate);
    const glm::vec2 below = world.screen(gizmo.pivot) + glm::vec2(0.0f, 40.0f);

    Manipulation drag;
    REQUIRE(drag.begin(*box, gizmo, Handle::X, world.view(), below));
    REQUIRE(drag.drag(world.scene, world.view(), below + glm::vec2(0.0f, 30.0f), false));
    CHECK(box->transform()->rotation().x > 0.1f);
    CHECK(box->transform()->rotation().y == 0.0f);
}

TEST_CASE("an axis scale changes one component and a uniform scale all three") {
    World world;
    Node* box = world.add("MeshPart", glm::vec3(0.0f));
    box->transform()->setRotation(0.0f, 0.0f, glm::radians(90.0f));
    box->transform()->setScale(1.0f, 2.0f, 3.0f);

    const Gizmo gizmo = world.gizmo(*box, Tool::Scale);
    const glm::vec3 tip = gizmo.pivot + gizmo.axes[0] * gizmo.length;
    CHECK(world.hit(gizmo, tip) == Handle::X);

    Manipulation drag;
    REQUIRE(world.grab(drag, *box, gizmo, Handle::X, tip));
    REQUIRE(world.to(drag, gizmo.pivot + gizmo.axes[0] * gizmo.length * 1.5f));
    CHECK(box->transform()->scale().x == doctest::Approx(1.5f).epsilon(1e-3));
    CHECK(box->transform()->scale().y == 2.0f);
    CHECK(box->transform()->scale().z == 3.0f);
    drag.end();

    box->transform()->setScale(1.0f, 2.0f, 3.0f);
    const glm::vec2 centre = world.screen(gizmo.pivot);
    CHECK(hitHandle(shapes(gizmo, world.view()), world.view(), centre) == Handle::Uniform);
    REQUIRE(drag.begin(*box, gizmo, Handle::Uniform, world.view(), centre));
    REQUIRE(drag.drag(world.scene, world.view(), centre + glm::vec2(cinder::dev::GIZMO_POINTS * 0.5f, 0.0f), false));
    CHECK(box->transform()->scale().x == doctest::Approx(1.5f));
    CHECK(box->transform()->scale().y == doctest::Approx(3.0f));
    CHECK(box->transform()->scale().z == doctest::Approx(4.5f));
}

TEST_CASE("snapping rounds the change, not the value") {
    World world;
    Node* box = world.add("MeshPart", glm::vec3(0.25f, 0.0f, 0.0f));

    const Gizmo move = world.gizmo(*box, Tool::Move);
    const glm::vec3 grab = move.pivot + glm::vec3(move.length * 0.8f, 0.0f, 0.0f);
    Manipulation drag;
    REQUIRE(world.grab(drag, *box, move, Handle::X, grab));
    REQUIRE(world.to(drag, grab + glm::vec3(2.4f, 0.0f, 0.0f), true));
    CHECK(box->transform()->position().x == 2.25f);
    drag.end();

    const Gizmo rotate = world.gizmo(*box, Tool::Rotate);
    const glm::vec3 normal(0.0f, 0.0f, 1.0f);
    REQUIRE(world.grab(drag, *box, rotate, Handle::Z, onRing(rotate, normal, 0.0f)));
    REQUIRE(world.to(drag, onRing(rotate, normal, 50.0f), true));
    CHECK(box->transform()->rotation().z == doctest::Approx(glm::radians(45.0f)));
    drag.end();

    const Gizmo scale = world.gizmo(*box, Tool::Scale);
    const glm::vec3 tip = scale.pivot + scale.axes[1] * scale.length;
    REQUIRE(world.grab(drag, *box, scale, Handle::Y, tip));
    REQUIRE(world.to(drag, scale.pivot + scale.axes[1] * scale.length * 1.26f, true));
    CHECK(box->transform()->scale().y == doctest::Approx(1.3f));
}

TEST_CASE("cancelling restores the transform exactly and leaves no undo step") {
    World world;
    Node* box = world.add("MeshPart", glm::vec3(0.1f, 0.2f, 0.3f));
    box->transform()->setRotation(0.4f, 0.5f, 0.6f);
    box->transform()->setScale(1.1f, 1.2f, 1.3f);
    const glm::vec3 position = box->transform()->position();
    const glm::vec3 rotation = box->transform()->rotation();
    const glm::vec3 scale = box->transform()->scale();

    History history(world.scene);
    history.reset();

    const Gizmo gizmo = world.gizmo(*box, Tool::Move);
    const glm::vec3 grab = gizmo.pivot + glm::vec3(gizmo.length * 0.8f, 0.0f, 0.0f);
    Manipulation drag;
    REQUIRE(world.grab(drag, *box, gizmo, Handle::X, grab));
    REQUIRE(world.to(drag, grab + glm::vec3(2.0f, 0.0f, 0.0f)));
    history.touch("Move Box", box->id());
    CHECK(box->transform()->position().x != position.x);

    REQUIRE(drag.cancel(world.scene));
    CHECK_FALSE(drag.active());
    history.settle(false);

    CHECK(history.steps() == 0);
    CHECK_FALSE(history.dirty());
    CHECK(box->transform()->position() == position);
    CHECK(box->transform()->rotation() == rotation);
    CHECK(box->transform()->scale() == scale);
}

TEST_CASE("a press on a handle that never moves records no undo step") {
    World world;
    Node* group = world.add("Group", glm::vec3(1.0f, 2.0f, -3.0f));
    group->transform()->setRotation(0.3f, 0.7f, 0.1f);
    group->transform()->setScale(1.7f, 0.6f, 1.3f);
    Node* box = world.add("MeshPart", glm::vec3(0.3f, -0.2f, 0.9f), group);
    box->transform()->setRotation(0.2f, 0.1f, 0.4f);

    History history(world.scene);
    history.reset();

    for (Tool tool : {Tool::Move, Tool::Rotate, Tool::Scale}) {
        const Gizmo gizmo = world.gizmo(*box, tool, Space::Local);
        const glm::vec3 grab = tool == Tool::Rotate ? onRing(gizmo, gizmo.axes[0], 30.0f)
                                                    : gizmo.pivot + gizmo.axes[0] * gizmo.length * 0.9f;
        Manipulation drag;
        REQUIRE(world.grab(drag, *box, gizmo, Handle::X, grab));
        CHECK(world.to(drag, grab));
        history.touch("Edit", box->id());
        drag.end();
        history.settle(false);
    }
    CHECK(history.steps() == 0);
    CHECK_FALSE(history.dirty());
}

TEST_CASE("handles keep their size on screen at any distance") {
    World world;
    for (float z : {0.0f, -40.0f}) {
        CAPTURE(z);
        Node* box = world.add("MeshPart", glm::vec3(0.0f, 0.0f, z));
        const Gizmo gizmo = world.gizmo(*box, Tool::Move);
        const float length = glm::distance(world.screen(gizmo.pivot),
                                           world.screen(gizmo.pivot + glm::vec3(gizmo.length, 0.0f, 0.0f)));
        CHECK(length == doctest::Approx(cinder::dev::GIZMO_POINTS).epsilon(1e-3));
    }
}

TEST_CASE("there is no gizmo under a zero-scale parent or behind the camera") {
    World world;
    Node* group = world.add("Group", glm::vec3(0.0f));
    group->transform()->setScale(0.0f);
    Node* flat = world.add("MeshPart", glm::vec3(0.0f), group);
    CHECK_FALSE(gizmoFor(*flat->transform(), Tool::Move, Space::World, world.view()));

    Node* behind = world.add("MeshPart", glm::vec3(0.0f, 0.0f, 20.0f));
    CHECK_FALSE(gizmoFor(*behind->transform(), Tool::Move, Space::World, world.view()));
}

TEST_CASE("a drag ends when its node is destroyed") {
    World world;
    Node* box = world.add("MeshPart", glm::vec3(0.0f));
    const Gizmo gizmo = world.gizmo(*box, Tool::Move);
    const glm::vec3 grab = gizmo.pivot + glm::vec3(gizmo.length * 0.8f, 0.0f, 0.0f);

    Manipulation drag;
    REQUIRE(world.grab(drag, *box, gizmo, Handle::X, grab));
    world.scene.destroyNow(box);
    CHECK_FALSE(world.to(drag, grab + glm::vec3(1.0f, 0.0f, 0.0f)));
    CHECK_FALSE(drag.active());
}

TEST_CASE("a handle that does not belong to the tool cannot be grabbed") {
    World world;
    Node* box = world.add("MeshPart", glm::vec3(0.0f));
    Manipulation drag;
    CHECK_FALSE(drag.begin(*box, world.gizmo(*box, Tool::Rotate), Handle::XY, world.view(), SIZE * 0.5f));
    CHECK_FALSE(drag.begin(*box, world.gizmo(*box, Tool::Scale), Handle::View, world.view(), SIZE * 0.5f));
    CHECK_FALSE(drag.active());

    Node* folder = world.scene.create("Folder", nullptr);
    CHECK_FALSE(drag.begin(*folder, world.gizmo(*box, Tool::Move), Handle::X, world.view(), SIZE * 0.5f));
}
