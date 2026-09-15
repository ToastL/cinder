#include <doctest/doctest.h>

#include "components/Builtins.hpp"
#include "components/Sprite.hpp"
#include "dev/Picking.hpp"
#include "gfx/pass/PerspectiveCamera.hpp"
#include "scene/Node.hpp"
#include "scene/NodeTypes.hpp"
#include "scene/Scene.hpp"
#include "scene/Transform.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/trigonometric.hpp>

#include <optional>
#include <string_view>

using cinder::components::Sprite;
using cinder::dev::PickView;
using cinder::dev::Ray;
using cinder::dev::hitCube;
using cinder::dev::hitSprite;
using cinder::dev::rayThrough;
using cinder::scene::Node;
using cinder::scene::NodeTypes;
using cinder::scene::Scene;

namespace {

const glm::vec2 SIZE(200.0f, 100.0f);
const glm::vec2 CENTRE(100.0f, 50.0f);
const glm::vec2 NOWHERE(1.0e6f, 1.0e6f);

struct World {
    NodeTypes types;
    Scene scene{types};
    cinder::gfx::pass::PerspectiveCamera camera;

    World() {
        cinder::components::registerBuiltins(types);
        camera.setFov(glm::radians(60.0f));
        camera.setAspect(SIZE.x / SIZE.y);
        camera.setClip(0.1f, 100.0f);
    }

    Node* add(std::string_view className, float z, Node* parent = nullptr) {
        Node* node = scene.create(className, parent);
        node->transform()->setPosition(0.0f, 0.0f, z);
        return node;
    }

    Node* pickAt(glm::vec2 point, glm::vec2 world2d = NOWHERE) {
        return cinder::dev::pick(scene, PickView{camera.viewProjection(), point, SIZE, world2d});
    }
};

}

TEST_CASE("a ray through the view starts on the near plane and points into the screen") {
    World world;
    const Ray centre = rayThrough(world.camera.viewProjection(), CENTRE, SIZE);
    CHECK(centre.origin.z == doctest::Approx(-0.1f));
    CHECK(centre.direction.x == doctest::Approx(0.0f));
    CHECK(centre.direction.z == doctest::Approx(-1.0f));

    const Ray top = rayThrough(world.camera.viewProjection(), glm::vec2(CENTRE.x, 0.0f), SIZE);
    CHECK(top.direction.y > 0.0f);
}

TEST_CASE("a ray hits a cube at the distance of its near face") {
    const glm::mat4 world = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -5.0f));

    const std::optional<float> distance = hitCube(Ray{}, world);
    REQUIRE(distance);
    CHECK(*distance == doctest::Approx(4.5f));

    CHECK_FALSE(hitCube(Ray{glm::vec3(2.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)}, world));
    CHECK_FALSE(hitCube(Ray{glm::vec3(0.0f, 0.0f, -10.0f), glm::vec3(0.0f, 0.0f, -1.0f)}, world));
}

TEST_CASE("a cube is hit through its rotation and scale, and never at zero scale") {
    const glm::mat4 stretched = glm::scale(glm::mat4(1.0f), glm::vec3(4.0f, 1.0f, 1.0f));
    const glm::mat4 turned =
            glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)) * stretched;
    const Ray ray{glm::vec3(3.0f, 0.0f, 1.5f), glm::vec3(-1.0f, 0.0f, 0.0f)};

    const std::optional<float> distance = hitCube(ray, turned);
    REQUIRE(distance);
    CHECK(*distance == doctest::Approx(2.5f));
    CHECK_FALSE(hitCube(ray, stretched));
    CHECK_FALSE(hitCube(Ray{}, glm::scale(glm::mat4(1.0f), glm::vec3(0.0f))));
}

TEST_CASE("the nearer of two cubes is picked") {
    World world;
    world.add("MeshPart", -10.0f);
    Node* nearer = world.add("MeshPart", -5.0f);
    CHECK(world.pickAt(CENTRE) == nearer);
    CHECK(world.pickAt(glm::vec2(0.0f, 0.0f)) == nullptr);
}

TEST_CASE("a sprite under the cursor wins over a cube") {
    World world;
    Node* cube = world.add("MeshPart", -5.0f);
    Sprite* sprite = world.scene.create<Sprite>(nullptr);
    sprite->transform()->setPosition(20.0f, 30.0f, 0.0f);

    CHECK(world.pickAt(CENTRE, glm::vec2(20.0f, 30.0f)) == sprite);
    CHECK(world.pickAt(CENTRE) == cube);
}

TEST_CASE("a rotated sprite is hit inside its turned rectangle") {
    World world;
    Sprite* sprite = world.scene.create<Sprite>(nullptr);
    sprite->setSize(10.0f, 2.0f);
    sprite->transform()->setRotation(0.0f, 0.0f, glm::radians(90.0f));
    const glm::mat4& placed = sprite->transform()->world();

    CHECK(hitSprite(glm::vec2(0.0f, 4.0f), placed, sprite->size()));
    CHECK_FALSE(hitSprite(glm::vec2(4.0f, 0.0f), placed, sprite->size()));
}

TEST_CASE("a disabled node hides its subtree from picking") {
    World world;
    Node* group = world.add("Group", 0.0f);
    Node* cube = world.add("MeshPart", -5.0f, group);

    group->setEnabled(false);
    CHECK(world.pickAt(CENTRE) == nullptr);

    group->setEnabled(true);
    CHECK(world.pickAt(CENTRE) == cube);
}

TEST_CASE("a perspective camera is picked near its projected position") {
    World world;
    Node* camera = world.add("Camera", -5.0f);
    CHECK(world.pickAt(CENTRE) == camera);
    CHECK(world.pickAt(CENTRE + glm::vec2(40.0f, 0.0f)) == nullptr);
}
