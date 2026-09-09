#include <doctest/doctest.h>

#include "scene/Actor.hpp"
#include "scene/Component.hpp"
#include "scene/Components.hpp"
#include "scene/Scene.hpp"

using cinder::scene::Actor;
using cinder::scene::Component;
using cinder::scene::Components;
using cinder::scene::Scene;

namespace {

struct Counter : Component {
    int starts = 0;
    int updates = 0;
    int destroys = 0;

    void onStart() override { starts++; }
    void onUpdate(float dt) override { updates++; }
    void onDestroy() override { destroys++; }

    CINDER_COMPONENT(Counter, Component) {}
};

struct Suicide : Component {
    void onUpdate(float dt) override { actor()->destroy(); }

    CINDER_COMPONENT(Suicide, Component) {}
};

struct Spawner : Component {
    Counter* spawned = nullptr;

    void onUpdate(float dt) override {
        if (spawned == nullptr) spawned = actor()->spawnChild("child")->add<Counter>();
    }

    CINDER_COMPONENT(Spawner, Component) {}
};

struct Fixture {
    Components types;
    Scene scene{types};
};

}

TEST_CASE("start runs once before the first update") {
    Fixture f;
    Counter* counter = f.scene.spawn("a")->add<Counter>();

    f.scene.update(0.1f);
    CHECK(counter->starts == 1);
    CHECK(counter->updates == 1);

    f.scene.update(0.1f);
    CHECK(counter->starts == 1);
    CHECK(counter->updates == 2);
}

TEST_CASE("components added during update start on the next frame") {
    Fixture f;
    Spawner* spawner = f.scene.spawn("spawner")->add<Spawner>();

    f.scene.update(0.1f);
    CHECK(spawner->spawned->starts == 0);
    CHECK(spawner->spawned->updates == 0);

    f.scene.update(0.1f);
    CHECK(spawner->spawned->starts == 1);
    CHECK(spawner->spawned->updates == 1);
}

TEST_CASE("destroy is deferred to the end of the frame") {
    Fixture f;
    Actor* a = f.scene.spawn("a");
    const int id = a->id();
    a->add<Suicide>();
    Counter* sibling = f.scene.spawn("b")->add<Counter>();

    f.scene.update(0.1f);
    CHECK(sibling->updates == 1);
    CHECK(f.scene.byId(id) == nullptr);
}

TEST_CASE("destroying a parent destroys its children") {
    Fixture f;
    Actor* parent = f.scene.spawn("parent");
    Actor* child = f.scene.spawn("child", parent);
    const int childId = child->id();
    Counter* counter = child->add<Counter>();

    f.scene.update(0.1f);
    CHECK(counter->starts == 1);

    f.scene.destroy(parent);
    f.scene.update(0.1f);

    CHECK(f.scene.byId(childId) == nullptr);
    CHECK(f.scene.roots().empty());
}

TEST_CASE("disabled components start but do not update") {
    Fixture f;
    Counter* counter = f.scene.spawn("a")->add<Counter>();
    counter->setEnabled(false);

    f.scene.update(0.1f);
    CHECK(counter->starts == 1);
    CHECK(counter->updates == 0);
}

TEST_CASE("inactive actors skip their whole subtree") {
    Fixture f;
    Actor* parent = f.scene.spawn("parent");
    Counter* counter = f.scene.spawn("child", parent)->add<Counter>();
    parent->setActive(false);

    f.scene.update(0.1f);
    CHECK(counter->starts == 1);
    CHECK(counter->updates == 0);
    CHECK_FALSE(counter->actor()->active());
}

TEST_CASE("ids are stable and lookups resolve") {
    Fixture f;
    Actor* a = f.scene.spawn("a");
    Actor* b = f.scene.spawn("b", a);

    CHECK(a->id() != b->id());
    CHECK(f.scene.byId(a->id()) == a);
    CHECK(f.scene.find("b") == b);
    CHECK(f.scene.find("nope") == nullptr);
}

TEST_CASE("an explicit id is honoured and raises the watermark") {
    Fixture f;
    Actor* loaded = f.scene.spawn(42, "loaded", nullptr);

    CHECK(loaded->id() == 42);
    CHECK(f.scene.byId(42) == loaded);
    CHECK(f.scene.spawn("next")->id() == 43);
}

TEST_CASE("a taken id is rejected") {
    Fixture f;
    Actor* a = f.scene.spawn("a");
    CHECK_THROWS_AS(f.scene.spawn(a->id(), "clash", nullptr), std::runtime_error);
}

TEST_CASE("an explicit id below the watermark does not lower it") {
    Fixture f;
    f.scene.spawn(10, "ten", nullptr);
    f.scene.spawn(3, "three", nullptr);

    CHECK(f.scene.spawn("next")->id() == 11);
}

TEST_CASE("clear does not reissue ids") {
    Fixture f;
    const int before = f.scene.spawn("before")->id();

    f.scene.clear();
    Actor* after = f.scene.spawn("after");

    CHECK(before != after->id());
    CHECK(f.scene.byId(before) == nullptr);
}

TEST_CASE("a component belongs to exactly one actor") {
    Fixture f;
    Actor* a = f.scene.spawn("a");
    Counter* counter = a->add<Counter>();

    CHECK(counter->actor() == a);
    CHECK(a->components().size() == 1);
    CHECK(a->add(nullptr) == nullptr);
    CHECK(a->components().size() == 1);
}

TEST_CASE("removing a component clears its pending start") {
    Fixture f;
    Actor* a = f.scene.spawn("a");
    Counter* counter = a->add<Counter>();

    a->remove(counter);
    f.scene.update(0.1f);

    CHECK(a->components().empty());
}
