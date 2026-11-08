#include <doctest/doctest.h>

#include "test_support.hpp"

#include "Components/Actors.hpp"
#include "Components/SpriteComponent.hpp"
#include "Components/StaticMeshComponent.hpp"
#include "Dev/History.hpp"
#include "Dev/Selection.hpp"
#include "Platform/Assets.hpp"
#include "Platform/Files.hpp"
#include "Scene/PrefabCache.hpp"
#include "Scene/Ref.hpp"
#include "Scene/Transform.hpp"
#include "Serial/Prefab.hpp"
#include "Serial/SceneCodec.hpp"

#include <filesystem>
#include <functional>
#include <string>

using cinder::Components::SpriteComponent;
using cinder::Components::StaticMeshActor;
using cinder::Scene::Actor;
using cinder::Scene::Component;
using cinder::Scene::Ref;
using cinder::Scene::Scene;
using cinder::Scene::SceneComponent;
using cinder::Serial::Prefab;
using cinder::Serial::SceneCodec;

namespace {

struct Aim final : Component {
    Ref<Actor> target_;
    float speed_ = 1.0f;

    CINDER_COMPONENT(Aim, Component) {
        CINDER_PROP(target_);
        CINDER_PROP(speed_);
    }
};

constexpr const char* BOX = "Prefabs/box.prefab";

struct Project : testing::SceneFixture {
    std::filesystem::path root = testing::uniqueTempDir("cinder_prefab_test");

    Project() {
        types.add<Aim>("Aim");
        std::filesystem::create_directories(root / "Content" / "Prefabs");
        cinder::Platform::setProjectRoot(root);
        writeBox(2.0f, true, false);
    }

    ~Project() { std::filesystem::remove_all(root); }

    void writePrefab(const char* path, const std::function<void(Scene&)>& build) {
        Scene other(types);
        build(other);
        cinder::Platform::writeTextFile(cinder::Platform::contentPath(path), SceneCodec::save(other));
    }

    void writeBox(float speed, bool barrel, bool scope) {
        writePrefab(BOX, [&](Scene& other) {
            auto* box = other.spawn<StaticMeshActor>();
            box->setName("Box");
            auto* aim = static_cast<Aim*>(other.addComponent(*box, "Aim"));
            aim->speed_ = speed;
            Actor* inner = other.insert(std::make_unique<Actor>(), box, 4, {});
            inner->setName("Barrel");
            other.addComponent(*inner, "SceneComponent");
            if (barrel) aim->target_ = Ref<Actor>(inner);
            else other.destroyNow(inner);
            if (scope) other.insert(std::make_unique<Actor>(), box, 10, {})->setName("Scope");
        });
    }

    Actor* place(Actor* parent = nullptr) { return Prefab::instantiate(scene, BOX, parent); }

    std::string reload() {
        const std::string text = SceneCodec::save(scene);
        SceneCodec::load(text, scene);
        return text;
    }

    Actor* byName(const char* name) const { return scene.find(name); }
};

Aim* aimOf(Actor* actor) { return actor == nullptr ? nullptr : actor->findComponent<Aim>(); }

}

TEST_CASE("an instance saves its link and only what differs from the prefab") {
    Project p;
    Actor* box = p.place();
    REQUIRE(box != nullptr);
    CHECK(box->prefab() == BOX);
    box->transform()->setPosition(5.0f, 0.0f, 0.0f);

    const std::string text = SceneCodec::save(p.scene);
    CHECK(text.find("prefab \"Prefabs/box.prefab\"") != std::string::npos);
    CHECK(text.find("from 2") != std::string::npos);
    CHECK(text.find("position 5 0 0") != std::string::npos);
    CHECK(text.find("speed") == std::string::npos);
    CHECK(text.find("target") == std::string::npos);
    CHECK(text.find("name") == std::string::npos);

    SceneCodec::load(text, p.scene);
    CHECK(SceneCodec::save(p.scene) == text);
    CHECK(aimOf(p.byName("Box"))->speed_ == 2.0f);
}

TEST_CASE("editing the prefab reaches every instance that does not override the field") {
    Project p;
    p.place()->setName("A");
    aimOf(p.place())->speed_ = 7.0f;
    const std::string text = SceneCodec::save(p.scene);

    p.writeBox(3.0f, true, false);
    SceneCodec::load(text, p.scene);
    CHECK(aimOf(p.byName("A"))->speed_ == 3.0f);
    CHECK(aimOf(p.byName("Box"))->speed_ == 7.0f);
}

TEST_CASE("a reference inside a prefab points at each instance's own copy") {
    Project p;
    Actor* a = p.place();
    a->setName("A");
    Actor* b = p.place();
    b->setName("B");
    CHECK(aimOf(a)->target_.get(p.scene) == a->findAttached("Barrel"));
    CHECK(aimOf(b)->target_.get(p.scene) == b->findAttached("Barrel"));

    p.reload();
    a = p.byName("A");
    b = p.byName("B");
    CHECK(aimOf(a)->target_.get(p.scene) == a->findAttached("Barrel"));
    CHECK(aimOf(b)->target_.get(p.scene) == b->findAttached("Barrel"));

    aimOf(b)->target_ = Ref<Actor>();
    const std::string text = p.reload();
    CHECK(text.find("target 0") != std::string::npos);
    CHECK(aimOf(p.byName("B"))->target_.get(p.scene) == nullptr);
}

TEST_CASE("references into and out of an instance survive a reload") {
    Project p;
    Actor* box = p.place();
    Actor* watcher = p.scene.spawn("Actor");
    watcher->setName("Watcher");
    static_cast<Aim*>(p.scene.addComponent(*watcher, "Aim"))->target_ = Ref<Actor>(box->findAttached("Barrel"));
    aimOf(box)->target_ = Ref<Actor>(watcher);

    p.reload();
    box = p.byName("Box");
    CHECK(aimOf(p.byName("Watcher"))->target_.get(p.scene) == box->findAttached("Barrel"));
    CHECK(aimOf(box)->target_.get(p.scene) == p.byName("Watcher"));
}

TEST_CASE("an instance keeps what it adds, and what the prefab supplies comes back") {
    Project p;
    Actor* box = p.place();
    p.scene.addComponent(*box, "SpriteComponent");
    p.scene.spawn("Actor", box)->setName("Extra");
    p.scene.destroyNow(aimOf(box));

    p.reload();
    box = p.byName("Box");
    CHECK(box->findComponent<SpriteComponent>() != nullptr);
    CHECK(box->findAttached("Extra") != nullptr);
    REQUIRE(aimOf(box) != nullptr);
    CHECK(aimOf(box)->target_.get(p.scene) == box->findAttached("Barrel"));
    CHECK(box->findComponent<SpriteComponent>()->prefabId() == 0);
}

TEST_CASE("an instance of a missing prefab round-trips verbatim and comes back with the file") {
    Project p;
    Actor* box = p.place();
    box->transform()->setPosition(1.0f, 2.0f, 3.0f);
    aimOf(box)->speed_ = 5.0f;
    p.scene.addComponent(*box, "SpriteComponent");
    const std::string text = SceneCodec::save(p.scene);

    const std::string prefab = cinder::Platform::readTextFile(cinder::Platform::contentPath(BOX));
    std::filesystem::remove(cinder::Platform::contentPath(BOX));
    p.types.prefabs().clear();
    SceneCodec::load(text, p.scene);
    CHECK(SceneCodec::save(p.scene) == text);

    cinder::Platform::writeTextFile(cinder::Platform::contentPath(BOX), prefab);
    p.types.prefabs().clear();
    SceneCodec::load(text, p.scene);
    box = p.byName("Box");
    REQUIRE(dynamic_cast<StaticMeshActor*>(box) != nullptr);
    CHECK(aimOf(box)->speed_ == 5.0f);
    CHECK(box->transform()->position() == glm::vec3(1.0f, 2.0f, 3.0f));
    CHECK(SceneCodec::save(p.scene) == text);
}

TEST_CASE("a prefab gaining or losing a child changes its instances") {
    Project p;
    p.place();
    const std::string text = SceneCodec::save(p.scene);

    p.writeBox(2.0f, true, true);
    SceneCodec::load(text, p.scene);
    CHECK(p.byName("Box")->findAttached("Scope") != nullptr);
    CHECK(p.byName("Box")->findAttached("Barrel") != nullptr);

    p.writeBox(2.0f, false, false);
    SceneCodec::load(text, p.scene);
    CHECK(p.byName("Box")->findAttached("Barrel") == nullptr);
    CHECK(aimOf(p.byName("Box"))->target_.get(p.scene) == nullptr);
}

TEST_CASE("Apply writes an instance into its prefab and rebases the others") {
    Project p;
    Actor* a = p.place();
    a->setName("A");
    a->transform()->setPosition(4.0f, 0.0f, 0.0f);
    aimOf(a)->speed_ = 9.0f;
    p.scene.addComponent(*a, "SpriteComponent");
    const int other = p.place()->id();
    const int applied = a->id();

    REQUIRE(Prefab::refusal(p.scene, *a, BOX).empty());
    REQUIRE(Prefab::write(p.scene, *a, BOX));

    Actor* b = p.scene.actor(other);
    a = p.scene.actor(applied);
    REQUIRE(a != nullptr);
    REQUIRE(b != nullptr);
    CHECK(a->prefab() == BOX);
    CHECK(aimOf(b)->speed_ == 9.0f);
    CHECK(b->findComponent<SpriteComponent>() != nullptr);
    CHECK(b->name() == "Box");
    CHECK(b->transform()->position() == glm::vec3(0.0f));
    CHECK(a->name() == "A");
    CHECK(a->transform()->position() == glm::vec3(4.0f, 0.0f, 0.0f));

    const std::string file = cinder::Platform::readTextFile(cinder::Platform::contentPath(BOX));
    CHECK(file.find("speed 9") != std::string::npos);
    CHECK(file.find("position") == std::string::npos);

    const std::string text = p.reload();
    CHECK(text.find("speed") == std::string::npos);
    CHECK(p.scene.actor(applied)->findComponent<SpriteComponent>()->prefabId() != 0);
}

TEST_CASE("Revert drops overrides and additions but keeps the name, the placement and the ids") {
    Project p;
    Actor* box = p.place();
    box->setName("Mine");
    box->transform()->setPosition(3.0f, 0.0f, 0.0f);
    aimOf(box)->speed_ = 9.0f;
    p.scene.addComponent(*box, "SpriteComponent");
    p.scene.spawn("Actor", box)->setName("Extra");
    const int id = box->id();
    const int aim = aimOf(box)->id();

    REQUIRE(Prefab::revert(p.scene, *box));
    box = p.scene.actor(id);
    REQUIRE(box != nullptr);
    CHECK(box->name() == "Mine");
    CHECK(box->transform()->position() == glm::vec3(3.0f, 0.0f, 0.0f));
    CHECK(aimOf(box)->speed_ == 2.0f);
    CHECK(aimOf(box)->id() == aim);
    CHECK(box->findComponent<SpriteComponent>() == nullptr);
    CHECK(box->findAttached("Extra") == nullptr);
    CHECK(box->findAttached("Barrel") != nullptr);
}

TEST_CASE("duplicating an instance keeps the link, duplicating a part of one does not") {
    Project p;
    Actor* box = p.place();
    Actor* copy = p.scene.clone(*box, nullptr);
    CHECK(copy->prefab() == BOX);
    CHECK(aimOf(copy)->target_.get(p.scene) == copy->findAttached("Barrel"));

    Actor* barrel = p.scene.clone(*box->findAttached("Barrel"), nullptr);
    CHECK(barrel->prefabId() == 0);
    CHECK(barrel->prefab().empty());

    const std::string text = SceneCodec::save(p.scene);
    std::size_t links = 0;
    for (std::size_t at = text.find("prefab \""); at != std::string::npos; at = text.find("prefab \"", at + 1)) ++links;
    CHECK(links == 2);
}

TEST_CASE("Save as Prefab numbers the prefab from 1 and links the source") {
    Project p;
    Actor* crate = p.scene.spawn("StaticMeshActor");
    crate->setName("Crate");
    crate->transform()->setPosition(7.0f, 0.0f, 0.0f);
    p.scene.addComponent(*crate, "Aim");
    const int id = crate->id();

    REQUIRE(Prefab::refusal(p.scene, *crate, "Prefabs/crate.prefab").empty());
    REQUIRE(Prefab::write(p.scene, *crate, "Prefabs/crate.prefab"));
    const std::string file = cinder::Platform::readTextFile(cinder::Platform::contentPath("Prefabs/crate.prefab"));
    CHECK(file.find("id 1\n") != std::string::npos);
    CHECK(file.find("position") == std::string::npos);

    crate = p.scene.actor(id);
    REQUIRE(crate != nullptr);
    CHECK(crate->prefab() == "Prefabs/crate.prefab");
    CHECK(crate->transform()->position() == glm::vec3(7.0f, 0.0f, 0.0f));
    const std::string text = p.reload();
    CHECK(text.find("prefab \"Prefabs/crate.prefab\"") != std::string::npos);
    CHECK(text.find("position 7 0 0") != std::string::npos);
}

TEST_CASE("prefab operations refuse a part of an instance and nested prefabs") {
    Project p;
    Actor* box = p.place();
    CHECK_FALSE(Prefab::refusal(p.scene, *box->findAttached("Barrel"), "Prefabs/barrel.prefab").empty());

    Actor* holder = p.scene.spawn("Actor");
    p.place(holder);
    CHECK_FALSE(Prefab::refusal(p.scene, *holder, "Prefabs/holder.prefab").empty());

    p.place(box);
    CHECK_FALSE(Prefab::refusal(p.scene, *box, BOX).empty());
}

TEST_CASE("version 5 scenes still load") {
    Project p;
    SceneCodec::load("version 5\nactors {\n    Actor {\n        id 3\n        name \"Old\"\n    }\n}\n", p.scene);
    CHECK(p.scene.actor(3)->name() == "Old");
}

TEST_CASE("the template cache follows the file and is dropped with the registry") {
    Project p;
    const Actor* first = SceneCodec::prefab(p.types, BOX);
    REQUIRE(first != nullptr);
    CHECK(SceneCodec::prefab(p.types, BOX) == first);

    p.writeBox(6.0f, true, false);
    const Actor* second = SceneCodec::prefab(p.types, BOX);
    REQUIRE(second != nullptr);
    CHECK(second->findComponent<Aim>()->speed_ == 6.0f);

    p.types.add<Aim>("Aim");
    CHECK(p.types.prefabs().find(BOX) == nullptr);
}

TEST_CASE("undo restores an instance override and its link") {
    Project p;
    cinder::Dev::Selection selection;
    cinder::Dev::History history(p.scene);
    Actor* box = p.place();
    history.touch("Place", box->id());
    history.settle(false);

    history.touch("Edit", box->id());
    aimOf(box)->speed_ = 8.0f;
    history.settle(false);

    history.undo(selection);
    box = p.byName("Box");
    CHECK(box->prefab() == BOX);
    CHECK(aimOf(box)->speed_ == 2.0f);
    history.redo(selection);
    CHECK(aimOf(p.byName("Box"))->speed_ == 8.0f);
}
