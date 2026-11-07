#include <doctest/doctest.h>

#include "Dev/Picking.hpp"
#include "Gfx/Pass/ViewCamera.hpp"
#include "Components/Actors.hpp"
#include "Components/Builtins.hpp"
#include "Scene/Actor.hpp"
#include "Scene/ClassRegistry.hpp"
#include "Scene/SceneComponent.hpp"
#include "Scene/Scene.hpp"
#include "Scene/Transform.hpp"
#include "Scene/View.hpp"

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/trigonometric.hpp>

#include <optional>
#include <string_view>

using cinder::Components::CameraActor;
using cinder::Components::CameraComponent;
using cinder::Components::SpriteActor;
using cinder::Components::SpriteComponent;
using cinder::Components::StaticMeshComponent;
using cinder::Dev::PickView;
using cinder::Dev::Ray;
using cinder::Dev::hitCube;
using cinder::Dev::hitSprite;
using cinder::Dev::rayThrough;
using cinder::Scene::Actor;
using cinder::Scene::ClassRegistry;
using cinder::Scene::Component;
using cinder::Scene::Scene;

namespace {

const glm::vec2 SIZE(200.0f, 100.0f);
const glm::vec2 CENTRE(100.0f, 50.0f);

struct World {
    ClassRegistry types;
    Scene scene{types};
    cinder::Gfx::Pass::ViewCamera camera;

    World() {
        cinder::Components::registerBuiltins(types);
        cinder::Scene::View view;
        view.fovDegrees = 60.0f;
        view.nearClip = 0.1f;
        view.farClip = 100.0f;
        camera.setView(view);
        camera.setViewSize(SIZE.x, SIZE.y);
    }

    Actor* add(std::string_view className, float z, Actor* parent = nullptr) {
        Actor* actor = scene.spawn(className, parent);
        if (actor->rootComponent() == nullptr) actor->addComponent<cinder::Scene::SceneComponent>();
        actor->transform()->setPosition(0.0f, 0.0f, z);
        return actor;
    }

    Component* hitAt(glm::vec2 point) {
        return cinder::Dev::pick(scene, PickView{camera.viewProjection(), point, SIZE});
    }

    Actor* pickAt(glm::vec2 point) {
        Component* hit = hitAt(point);
        return hit != nullptr ? hit->owner() : nullptr;
    }
};

}

TEST_CASE("a ray through the view starts on the near plane and points into the screen") {
    World world;
    const Ray center = rayThrough(world.camera.viewProjection(), CENTRE, SIZE);
    CHECK(center.origin.z == doctest::Approx(-0.1f));
    CHECK(center.direction.x == doctest::Approx(0.0f));
    CHECK(center.direction.z == doctest::Approx(-1.0f));

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
    world.add("StaticMeshActor", -10.0f);
    Actor* nearer = world.add("StaticMeshActor", -5.0f);
    CHECK(world.pickAt(CENTRE) == nearer);
    CHECK(world.pickAt(glm::vec2(0.0f, 0.0f)) == nullptr);
}

TEST_CASE("a ray hits a sprite in its own plane, scaled, and never edge-on") {
    const glm::mat4 placed = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -5.0f)),
                                        glm::vec3(2.0f, 3.0f, 0.0f));

    const std::optional<float> distance = hitSprite(Ray{}, placed, glm::vec2(1.0f));
    REQUIRE(distance);
    CHECK(*distance == doctest::Approx(5.0f));

    CHECK(hitSprite(Ray{glm::vec3(0.9f, 1.4f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)}, placed, glm::vec2(1.0f)));
    CHECK_FALSE(hitSprite(Ray{glm::vec3(1.1f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)}, placed,
                          glm::vec2(1.0f)));
    CHECK_FALSE(hitSprite(Ray{glm::vec3(0.0f, 0.0f, -10.0f), glm::vec3(0.0f, 0.0f, -1.0f)}, placed,
                          glm::vec2(1.0f)));

    const glm::mat4 edgeOn =
            glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)) * placed;
    CHECK_FALSE(hitSprite(Ray{}, edgeOn, glm::vec2(1.0f)));
}

TEST_CASE("a rotated sprite is hit inside its turned rectangle") {
    World world;
    SpriteComponent* sprite = &world.scene.spawn<SpriteActor>()->sprite();
    sprite->setSize(10.0f, 2.0f);
    sprite->transform().setPosition(0.0f, 0.0f, -5.0f);
    sprite->transform().setRotation(0.0f, 0.0f, glm::radians(90.0f));
    const glm::mat4& placed = sprite->transform().world();

    CHECK(hitSprite(Ray{glm::vec3(0.0f), glm::normalize(glm::vec3(0.0f, 4.0f, -5.0f))}, placed,
                    sprite->size()));
    CHECK_FALSE(hitSprite(Ray{glm::vec3(0.0f), glm::normalize(glm::vec3(4.0f, 0.0f, -5.0f))}, placed,
                          sprite->size()));
}

TEST_CASE("sprites and cubes are picked by depth, and a later sprite in the same plane wins") {
    World world;
    Actor* cube = world.add("StaticMeshActor", -5.0f);
    Actor* front = world.add("SpriteActor", -3.0f);
    CHECK(world.pickAt(CENTRE) == front);

    Actor* above = world.add("SpriteActor", -3.0f);
    CHECK(world.pickAt(CENTRE) == above);

    front->transform()->setPosition(0.0f, 0.0f, -8.0f);
    above->transform()->setPosition(0.0f, 0.0f, -8.0f);
    CHECK(world.pickAt(CENTRE) == cube);
}

TEST_CASE("a disabled actor hides the actors attached to it from picking") {
    World world;
    Actor* group = world.add("Actor", 0.0f);
    Actor* cube = world.add("StaticMeshActor", -5.0f, group);

    group->setEnabled(false);
    CHECK(world.pickAt(CENTRE) == nullptr);

    group->setEnabled(true);
    CHECK(world.pickAt(CENTRE) == cube);
}

TEST_CASE("a camera of either projection is picked near its projected position") {
    World world;
    auto* camera = dynamic_cast<CameraActor*>(world.add("CameraActor", -5.0f));
    REQUIRE(camera != nullptr);
    CHECK(world.pickAt(CENTRE) == camera);
    CHECK(world.pickAt(CENTRE + glm::vec2(40.0f, 0.0f)) == nullptr);

    camera->camera().setProjection(CameraComponent::Projection::Orthographic);
    CHECK(world.pickAt(CENTRE) == camera);
}

TEST_CASE("a pick reports the component that was hit, which may not be the root") {
    World world;
    Actor* holder = world.add("Actor", 0.0f);
    auto* mesh = holder->addComponent<StaticMeshComponent>("Mesh");
    mesh->transform().setPosition(0.0f, 0.0f, -5.0f);

    CHECK(world.hitAt(CENTRE) == mesh);
    CHECK(world.pickAt(CENTRE) == holder);
    mesh->setEnabled(false);
    CHECK(world.hitAt(CENTRE) == nullptr);
}
