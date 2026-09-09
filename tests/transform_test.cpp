#include <doctest/doctest.h>

#include "scene/Actor.hpp"
#include "scene/Components.hpp"
#include "scene/Scene.hpp"
#include "scene/Transform.hpp"

#include <glm/gtc/constants.hpp>

using cinder::scene::Actor;
using cinder::scene::Components;
using cinder::scene::Scene;
using cinder::scene::Transform;

namespace {

struct Fixture {
    Components types;
    Scene scene{types};
};

void checkWorld(Actor* actor, float x, float y, float z) {
    const glm::vec3 p = actor->transform().worldPosition();
    CHECK(p.x == doctest::Approx(x).epsilon(1e-5));
    CHECK(p.y == doctest::Approx(y).epsilon(1e-5));
    CHECK(p.z == doctest::Approx(z).epsilon(1e-5));
}

}

TEST_CASE("world follows the parent") {
    Fixture f;
    Actor* parent = f.scene.spawn("parent");
    Actor* child = f.scene.spawn("child", parent);

    parent->transform().setPosition(10, 0, 0);
    child->transform().setPosition(0, 5, 0);
    checkWorld(child, 10, 5, 0);

    parent->transform().setPosition(0, 0, 3);
    checkWorld(child, 0, 5, 3);
}

TEST_CASE("parent scale scales the child offset") {
    Fixture f;
    Actor* parent = f.scene.spawn("parent");
    Actor* child = f.scene.spawn("child", parent);

    parent->transform().setScale(2);
    child->transform().setPosition(0, 5, 0);
    checkWorld(child, 0, 10, 0);
}

TEST_CASE("parent rotation rotates the child offset") {
    Fixture f;
    Actor* parent = f.scene.spawn("parent");
    Actor* child = f.scene.spawn("child", parent);

    parent->transform().setRotation(0, glm::half_pi<float>(), 0);
    child->transform().setPosition(0, 0, 1);
    checkWorld(child, 1, 0, 0);
}

TEST_CASE("reparenting recomputes the world matrix") {
    Fixture f;
    Actor* a = f.scene.spawn("a");
    a->transform().setPosition(10, 0, 0);
    Actor* b = f.scene.spawn("b");
    b->transform().setPosition(-10, 0, 0);

    Actor* child = f.scene.spawn("child", a);
    child->transform().setPosition(0, 1, 0);
    checkWorld(child, 10, 1, 0);

    child->setParent(b);
    checkWorld(child, -10, 1, 0);

    child->setParent(nullptr);
    checkWorld(child, 0, 1, 0);
}

TEST_CASE("three levels compose") {
    Fixture f;
    Actor* a = f.scene.spawn("a");
    Actor* b = f.scene.spawn("b", a);
    Actor* c = f.scene.spawn("c", b);

    a->transform().setPosition(1, 0, 0);
    b->transform().setPosition(0, 2, 0);
    c->transform().setPosition(0, 0, 4);
    checkWorld(c, 1, 2, 4);

    a->transform().setScale(2);
    checkWorld(c, 1, 4, 8);
}

TEST_CASE("in-place edits need an explicit dirty") {
    Fixture f;
    Actor* e = f.scene.spawn("e");
    checkWorld(e, 0, 0, 0);

    e->transform().position() = glm::vec3(5, 0, 0);
    checkWorld(e, 0, 0, 0);

    e->transform().dirty();
    checkWorld(e, 5, 0, 0);
}

TEST_CASE("writing position through a prop dirties the world matrix") {
    Fixture f;
    Actor* e = f.scene.spawn("e");
    checkWorld(e, 0, 0, 0);

    for (const cinder::reflect::PropDef& prop : cinder::reflect::props<Transform>()) {
        if (prop.name() != "position") continue;
        const float values[] = {5, 0, 0};
        prop.write(&e->transform(), values);
    }
    checkWorld(e, 5, 0, 0);
}

TEST_CASE("reparenting into its own subtree is rejected") {
    Fixture f;
    Actor* parent = f.scene.spawn("parent");
    Actor* child = f.scene.spawn("child", parent);

    CHECK_THROWS_AS(parent->setParent(child), std::runtime_error);
    CHECK_THROWS_AS(parent->setParent(parent), std::runtime_error);
}

TEST_CASE("transform props are position, rotation, scale in order") {
    const cinder::reflect::PropList& props = cinder::reflect::props<Transform>();
    REQUIRE(props.size() == 3);
    CHECK(props[0].name() == "position");
    CHECK(props[1].name() == "rotation");
    CHECK(props[2].name() == "scale");
}
