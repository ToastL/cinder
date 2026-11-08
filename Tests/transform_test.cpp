#include <doctest/doctest.h>

#include "Scene/Actor.hpp"
#include "Scene/ClassRegistry.hpp"
#include "Scene/Scene.hpp"
#include "Scene/SceneComponent.hpp"
#include "Scene/Transform.hpp"

#include <glm/gtc/constants.hpp>

#include <cmath>

using cinder::Scene::Actor;
using cinder::Scene::ClassRegistry;
using cinder::Scene::SceneComponent;
using cinder::Scene::Scene;
using cinder::Scene::Transform;

namespace {

struct Fixture {
    ClassRegistry types;
    Scene scene{types};

    Actor* spawn(const char* name, Actor* parent = nullptr) {
        Actor* actor = scene.spawn<Actor>(parent);
        actor->addComponent<SceneComponent>();
        actor->setName(name);
        return actor;
    }
};

void checkWorld(Transform& transform, float x, float y, float z) {
    const glm::vec3 p = transform.worldPosition();
    CHECK(p.x == doctest::Approx(x).epsilon(1e-5));
    CHECK(p.y == doctest::Approx(y).epsilon(1e-5));
    CHECK(p.z == doctest::Approx(z).epsilon(1e-5));
}

void checkWorld(Actor* actor, float x, float y, float z) { checkWorld(*actor->transform(), x, y, z); }

}

TEST_CASE("world follows the parent") {
    Fixture f;
    Actor* parent = f.spawn("parent");
    Actor* child = f.spawn("child", parent);

    parent->transform()->setPosition(10, 0, 0);
    child->transform()->setPosition(0, 5, 0);
    checkWorld(child, 10, 5, 0);

    parent->transform()->setPosition(0, 0, 3);
    checkWorld(child, 0, 5, 3);
}

TEST_CASE("parent scale scales the child offset") {
    Fixture f;
    Actor* parent = f.spawn("parent");
    Actor* child = f.spawn("child", parent);

    parent->transform()->setScale(2);
    child->transform()->setPosition(0, 5, 0);
    checkWorld(child, 0, 10, 0);
}

TEST_CASE("parent rotation rotates the child offset") {
    Fixture f;
    Actor* parent = f.spawn("parent");
    Actor* child = f.spawn("child", parent);

    parent->transform()->setRotation(0, glm::half_pi<float>(), 0);
    child->transform()->setPosition(0, 0, 1);
    checkWorld(child, 1, 0, 0);
}

TEST_CASE("reattaching recomputes the world matrix") {
    Fixture f;
    Actor* a = f.spawn("a");
    a->transform()->setPosition(10, 0, 0);
    Actor* b = f.spawn("b");
    b->transform()->setPosition(-10, 0, 0);

    Actor* child = f.spawn("child", a);
    child->transform()->setPosition(0, 1, 0);
    checkWorld(child, 10, 1, 0);

    child->attachTo(b);
    checkWorld(child, -10, 1, 0);

    child->attachTo(nullptr);
    checkWorld(child, 0, 1, 0);
}

TEST_CASE("three levels compose") {
    Fixture f;
    Actor* a = f.spawn("a");
    Actor* b = f.spawn("b", a);
    Actor* c = f.spawn("c", b);

    a->transform()->setPosition(1, 0, 0);
    b->transform()->setPosition(0, 2, 0);
    c->transform()->setPosition(0, 0, 4);
    checkWorld(c, 1, 2, 4);

    a->transform()->setScale(2);
    checkWorld(c, 1, 4, 8);
}

TEST_CASE("in-place edits need an explicit dirty") {
    Fixture f;
    Actor* e = f.spawn("e");
    checkWorld(e, 0, 0, 0);

    e->transform()->position() = glm::vec3(5, 0, 0);
    checkWorld(e, 0, 0, 0);

    e->transform()->dirty();
    checkWorld(e, 5, 0, 0);
}

TEST_CASE("writing position through a prop dirties the world matrix") {
    Fixture f;
    Actor* e = f.spawn("e");
    checkWorld(e, 0, 0, 0);

    for (const cinder::Reflect::PropDef& prop : cinder::Reflect::props<Transform>()) {
        if (prop.name() != "position") continue;
        const float values[] = {5, 0, 0};
        prop.write(e->transform(), values);
    }
    checkWorld(e, 5, 0, 0);
}

TEST_CASE("attaching into its own subtree is rejected") {
    Fixture f;
    Actor* parent = f.spawn("parent");
    Actor* child = f.spawn("child", parent);

    CHECK_THROWS_AS(parent->attachTo(child), std::runtime_error);
    CHECK_THROWS_AS(parent->attachTo(parent), std::runtime_error);
}

TEST_CASE("transform props are position, rotation, scale in order") {
    const cinder::Reflect::PropList& props = cinder::Reflect::props<Transform>();
    REQUIRE(props.size() == 3);
    CHECK(props[0].name() == "position");
    CHECK(props[1].name() == "rotation");
    CHECK(props[2].name() == "scale");
}

TEST_CASE("a component that is not the root composes with the root") {
    Fixture f;
    Actor* group = f.spawn("group");
    group->transform()->setPosition(10, 0, 0);
    auto* other = group->addComponent<SceneComponent>("Other");
    other->transform().setPosition(0, 1, 0);

    CHECK(group->rootComponent() != other);
    checkWorld(other->transform(), 10, 1, 0);
}

TEST_CASE("attached components compose through each other") {
    Fixture f;
    Actor* group = f.spawn("group");
    group->transform()->setScale(2);
    auto* arm = group->addComponent<SceneComponent>("Arm");
    auto* hand = group->addComponent<SceneComponent>("Hand");
    hand->attachTo(arm);
    arm->transform().setPosition(1, 0, 0);
    hand->transform().setPosition(0, 1, 0);

    checkWorld(hand->transform(), 2, 2, 0);
    hand->attachTo(nullptr);
    checkWorld(hand->transform(), 0, 2, 0);
}

TEST_CASE("a component attaches only inside its own actor") {
    Fixture f;
    Actor* a = f.spawn("a");
    Actor* b = f.spawn("b");
    CHECK_THROWS_AS(b->rootComponent()->attachTo(a->rootComponent()), std::runtime_error);
    CHECK_THROWS_AS(a->rootComponent()->attachTo(a->rootComponent()), std::runtime_error);
}

TEST_CASE("an attached actor without a root passes its parent's transform through") {
    Fixture f;
    Actor* top = f.spawn("top");
    top->transform()->setPosition(5, 0, 0);
    Actor* bare = f.scene.spawn<Actor>(top);
    Actor* leaf = f.spawn("leaf", bare);
    leaf->transform()->setPosition(0, 0, 1);

    CHECK(bare->rootComponent() == nullptr);
    checkWorld(leaf, 5, 0, 1);
}

TEST_CASE("rotate turns a component from where it already points") {
    Fixture f;
    Actor* box = f.spawn("box");
    box->transform()->setRotation(0, 1, 0);
    box->transform()->rotate(0, 2, 0);

    CHECK(box->transform()->rotation().y == doctest::Approx(3.0f));
}

TEST_CASE("rotation is declared as an angle, so the editor shows it in degrees") {
    const cinder::Reflect::PropList& defs = cinder::Reflect::props<cinder::Scene::Transform>();
    REQUIRE(defs.size() == 3);
    CHECK(defs[0].hint() == cinder::Reflect::PropHint::None);
    CHECK(defs[1].name() == "rotation");
    CHECK(defs[1].hint() == cinder::Reflect::PropHint::Angle);
}

TEST_CASE("a transform with no snapshot renders where it is") {
    Fixture f;
    Actor* box = f.spawn("Box");
    box->transform()->setPosition(3.0f, 0.0f, 0.0f);
    CHECK(box->transform()->interpolated(0.25f) == box->transform()->world());
}

TEST_CASE("rendering blends from the last step's pose by alpha") {
    Fixture f;
    Actor* box = f.spawn("Box");
    Transform& transform = *box->transform();
    transform.setPosition(0.0f, 0.0f, 0.0f);
    transform.snapshot();
    transform.setPosition(4.0f, 2.0f, 0.0f);
    transform.setRotation(0.0f, glm::half_pi<float>(), 0.0f);

    const glm::mat4 halfway = transform.interpolated(0.5f);
    CHECK(halfway[3].x == doctest::Approx(2.0f));
    CHECK(halfway[3].y == doctest::Approx(1.0f));
    const glm::vec3 axis(halfway[0]);
    CHECK(glm::length(axis) == doctest::Approx(1.0f));
    CHECK(axis.x == doctest::Approx(std::cos(glm::quarter_pi<float>())));
    CHECK(axis.z == doctest::Approx(-std::sin(glm::quarter_pi<float>())));

    CHECK(transform.interpolated(0.0f)[3].x == doctest::Approx(0.0f));
    CHECK(transform.interpolated(1.0f) == transform.world());
}

TEST_CASE("interpolation keeps a mirrored scale and blends scale linearly") {
    Fixture f;
    Transform& transform = *f.spawn("Box")->transform();
    transform.setScale(-1.0f, 1.0f, 1.0f);
    transform.snapshot();
    transform.setScale(-3.0f, 1.0f, 1.0f);
    const glm::mat4 halfway = transform.interpolated(0.5f);
    CHECK(halfway[0].x == doctest::Approx(-2.0f));
    CHECK(halfway[1].y == doctest::Approx(1.0f));
    CHECK(halfway[2].z == doctest::Approx(1.0f));
}

TEST_CASE("resetInterpolation makes a teleport render at once") {
    Fixture f;
    Transform& transform = *f.spawn("Box")->transform();
    transform.snapshot();
    transform.setPosition(100.0f, 0.0f, 0.0f);
    transform.resetInterpolation();
    CHECK(transform.interpolated(0.1f)[3].x == doctest::Approx(100.0f));
}

TEST_CASE("each update snapshots every transform, attached children and disabled actors included") {
    Fixture f;
    Actor* parent = f.spawn("Parent");
    Actor* child = f.spawn("Child", parent);
    child->transform()->setPosition(1.0f, 0.0f, 0.0f);
    parent->setEnabled(false);
    f.scene.update(1.0f / 60.0f);

    parent->transform()->setPosition(0.0f, 10.0f, 0.0f);
    CHECK(child->transform()->interpolated(0.5f)[3].y == doctest::Approx(5.0f));
    CHECK(child->transform()->interpolated(0.5f)[3].x == doctest::Approx(1.0f));
}

TEST_CASE("an actor spawned after the snapshot renders at its pose") {
    Fixture f;
    f.scene.update(1.0f / 60.0f);
    Actor* late = f.spawn("Late");
    late->transform()->setPosition(7.0f, 0.0f, 0.0f);
    CHECK(late->transform()->interpolated(0.0f)[3].x == doctest::Approx(7.0f));
}
