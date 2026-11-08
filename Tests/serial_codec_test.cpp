#include <doctest/doctest.h>

#include "Components/Actors.hpp"
#include "Components/Builtins.hpp"
#include "Scene/Actor.hpp"
#include "Scene/ClassRegistry.hpp"
#include "Scene/Placeholder.hpp"
#include "Scene/PropValue.hpp"
#include "Scene/Scene.hpp"
#include "Scene/SceneComponent.hpp"
#include "Serial/SceneCodec.hpp"

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using cinder::Components::CameraActor;
using cinder::Components::CameraComponent;
using cinder::Components::SpriteActor;
using cinder::Components::SpriteComponent;
using cinder::Components::StaticMeshActor;
using cinder::Components::StaticMeshComponent;
using cinder::Scene::Actor;
using cinder::Scene::ClassRegistry;
using cinder::Scene::Component;
using cinder::Scene::IdHint;
using cinder::Scene::PropSeq;
using cinder::Scene::PropValue;
using cinder::Scene::Scene;
using cinder::Scene::SceneComponent;
using cinder::Serial::SceneCodec;

namespace {

template <class T>
T* place(Scene& scene, int id, const char* name, Actor* parent, int rootId = 0) {
    auto owned = std::make_unique<T>();
    if (name != nullptr) owned->setName(name);
    std::vector<IdHint> hints;
    if (rootId != 0 && !owned->components().empty()) hints.push_back({owned->components().front(), rootId});
    return static_cast<T*>(scene.insert(std::move(owned), parent, id, hints));
}

Actor* group(Scene& scene, int id, const char* name, Actor* parent, int rootId) {
    auto owned = std::make_unique<Actor>();
    if (name != nullptr) owned->setName(name);
    Component* root = owned->addComponent<SceneComponent>();
    return scene.insert(std::move(owned), parent, id, {{root, rootId}});
}

struct Fixture {
    ClassRegistry types;
    Scene scene{types};

    Fixture() { cinder::Components::registerBuiltins(types); }

    void populate() {
        CameraActor* camera = place<CameraActor>(scene, 7, nullptr, nullptr, 8);
        camera->transform()->setPosition(0, 3, 8);
        camera->camera().setFov(70);

        SpriteActor* player = place<SpriteActor>(scene, 9, "Play\"er", nullptr, 10);
        player->transform()->setPosition(-1.5f, 0, 0.25f);
        player->sprite().setSize(64, 48).setColor(1, 0.2f, 0.2f, 1);
        player->setAttribute("hp", PropValue::integer(3));
        player->setAttribute("tag", PropValue::text("hero"));

        Actor* muzzle = group(scene, 12, "Muzzle", player, 13);
        muzzle->transform()->setPosition(0, 1, 0);
        muzzle->setEnabled(false);

        auto* tip = scene.addComponent(*player, std::make_unique<SceneComponent>(), "Tip", &player->sprite());
        static_cast<SceneComponent*>(tip)->transform().setPosition(0, 2, 0);
    }
};

struct Tuned : CameraActor {
    Tuned() { camera().setFov(30); }
    CINDER_ACTOR(Tuned, CameraActor) {}
};

bool contains(const std::string& text, std::string_view needle) {
    return text.find(needle) != std::string::npos;
}

}

TEST_CASE("the emitted format is pinned byte for byte") {
    Fixture f;
    f.populate();

    const std::string golden =
        "version 6\n"
        "actors {\n"
        "    CameraActor {\n"
        "        id 7\n"
        "        components {\n"
        "            CameraComponent {\n"
        "                id 8\n"
        "                position 0 3 8\n"
        "                fov 70\n"
        "            }\n"
        "        }\n"
        "    }\n"
        "    SpriteActor {\n"
        "        id 9\n"
        "        name \"Play\\\"er\"\n"
        "        components {\n"
        "            SpriteComponent {\n"
        "                id 10\n"
        "                position -1.5 0 0.25\n"
        "                size 64 48\n"
        "                color 1 0.2 0.2 1\n"
        "                attached {\n"
        "                    SceneComponent {\n"
        "                        id 14\n"
        "                        name \"Tip\"\n"
        "                        position 0 2 0\n"
        "                    }\n"
        "                }\n"
        "            }\n"
        "        }\n"
        "        attributes {\n"
        "            hp 3\n"
        "            tag \"hero\"\n"
        "        }\n"
        "        children {\n"
        "            Actor {\n"
        "                id 12\n"
        "                name \"Muzzle\"\n"
        "                enabled false\n"
        "                components {\n"
        "                    SceneComponent {\n"
        "                        id 13\n"
        "                        position 0 1 0\n"
        "                    }\n"
        "                }\n"
        "            }\n"
        "        }\n"
        "    }\n"
        "}\n";

    CHECK(SceneCodec::save(f.scene) == golden);
}

TEST_CASE("round trip is byte identical") {
    Fixture f;
    f.populate();

    const std::string first = SceneCodec::save(f.scene);
    SceneCodec::load(first, f.scene);

    CHECK(SceneCodec::save(f.scene) == first);
}

TEST_CASE("default valued props and names are omitted") {
    Fixture f;
    f.scene.spawn<SpriteActor>();

    const std::string text = SceneCodec::save(f.scene);

    CHECK_FALSE(contains(text, "enabled"));
    CHECK_FALSE(contains(text, "scale"));
    CHECK_FALSE(contains(text, "rotation"));
    CHECK_FALSE(contains(text, "texture"));
    CHECK_FALSE(contains(text, "size"));
    CHECK_FALSE(contains(text, "name"));
    CHECK(contains(text, "SpriteActor {"));
    CHECK(contains(text, "SpriteComponent {"));
}

TEST_CASE("a default component is written against the actor's own default, not its class default") {
    Fixture f;
    f.types.add<Tuned>("Tuned");
    f.scene.spawn<Tuned>();
    CHECK_FALSE(contains(SceneCodec::save(f.scene), "fov"));

    f.scene.clear();
    f.scene.spawn<Tuned>()->camera().setFov(60);
    const std::string text = SceneCodec::save(f.scene);
    CHECK(contains(text, "fov 60"));

    SceneCodec::load(text, f.scene);
    auto* loaded = dynamic_cast<Tuned*>(f.scene.roots().front());
    REQUIRE(loaded != nullptr);
    CHECK(loaded->camera().fov() == doctest::Approx(60));
    CHECK(loaded->components().size() == 1);
}

TEST_CASE("non-default props survive") {
    Fixture f;
    f.populate();
    SceneCodec::load(SceneCodec::save(f.scene), f.scene);

    CHECK(dynamic_cast<CameraComponent*>(f.scene.component(8))->fov() == doctest::Approx(70));

    const auto* sprite = dynamic_cast<SpriteComponent*>(f.scene.component(10));
    REQUIRE(sprite != nullptr);
    CHECK(sprite->size().x == doctest::Approx(64));
    CHECK(sprite->color().y == doctest::Approx(0.2f));
}

TEST_CASE("ids, names, classes, components and the hierarchy survive") {
    Fixture f;
    f.populate();
    SceneCodec::load(SceneCodec::save(f.scene), f.scene);

    CHECK(f.scene.actor(7)->name() == "CameraActor");
    CHECK(f.scene.actor(9)->name() == "Play\"er");
    CHECK(f.scene.byId(3) == nullptr);
    CHECK(f.scene.actor(12)->attachParent() == f.scene.actor(9));
    CHECK_FALSE(f.scene.actor(12)->isEnabled());
    CHECK(f.scene.roots().size() == 2);

    auto* player = dynamic_cast<SpriteActor*>(f.scene.actor(9));
    REQUIRE(player != nullptr);
    CHECK(&player->sprite() == f.scene.component(10));
    CHECK(player->sprite().isDefault());
    auto* tip = dynamic_cast<SceneComponent*>(f.scene.component(14));
    REQUIRE(tip != nullptr);
    CHECK(tip->owner() == player);
    CHECK(tip->attachParent() == &player->sprite());
    CHECK_FALSE(tip->isDefault());
    CHECK(player->components().size() == 2);
}

TEST_CASE("a component that is the root in the file stays the root") {
    Fixture f;
    auto owned = std::make_unique<StaticMeshActor>();
    auto* pivot = owned->addComponent<SceneComponent>("Pivot");
    owned->setRootComponent(pivot);
    owned->mesh().attachTo(pivot);
    owned->mesh().transform().setPosition(0, 1, 0);
    f.scene.insert(std::move(owned));

    const std::string text = SceneCodec::save(f.scene);
    SceneCodec::load(text, f.scene);
    auto* loaded = dynamic_cast<StaticMeshActor*>(f.scene.roots().front());
    REQUIRE(loaded != nullptr);
    REQUIRE(loaded->rootComponent() != nullptr);
    CHECK(loaded->rootComponent()->name() == "Pivot");
    CHECK(loaded->mesh().attachParent() == loaded->rootComponent());
    CHECK(SceneCodec::save(f.scene) == text);
}

TEST_CASE("load replaces the previous scene") {
    Fixture f;
    f.populate();
    SceneCodec::load("version 6\nactors {\n    Actor {\n        id 1\n    }\n}\n", f.scene);

    CHECK(f.scene.roots().size() == 1);
    CHECK(f.scene.byId(7) == nullptr);
}

TEST_CASE("a newer version is rejected and leaves the scene intact") {
    Fixture f;
    f.populate();

    CHECK_THROWS_AS(SceneCodec::load("version 99\nactors {\n}\n", f.scene), std::runtime_error);
    CHECK(f.scene.roots().size() == 2);
}

TEST_CASE("a missing version is rejected") {
    Fixture f;
    CHECK_THROWS_AS(SceneCodec::load("actors {\n}\n", f.scene), std::runtime_error);
}

TEST_CASE("node-tree files are rejected") {
    Fixture f;
    CHECK_THROWS_AS(SceneCodec::load("version 4\nnodes {\n}\n", f.scene), std::runtime_error);
    CHECK_THROWS_AS(SceneCodec::load("version 1\nactors {\n}\n", f.scene), std::runtime_error);
}

TEST_CASE("asset references survive by name") {
    Fixture f;
    place<StaticMeshActor>(f.scene, 1, "Box", nullptr, 3)->mesh().setMesh("sphere").setTexture("crate.png");
    place<SpriteActor>(f.scene, 2, "Tile", nullptr, 4)->sprite().setTexture("tile.png");
    SceneCodec::load(SceneCodec::save(f.scene), f.scene);

    auto* box = dynamic_cast<StaticMeshActor*>(f.scene.actor(1));
    REQUIRE(box != nullptr);
    CHECK(box->mesh().mesh() == "sphere");
    CHECK(box->mesh().texture() == "crate.png");
    CHECK(dynamic_cast<SpriteActor*>(f.scene.actor(2))->sprite().texture() == "tile.png");
}

TEST_CASE("camera render settings survive") {
    Fixture f;
    place<CameraActor>(f.scene, 1, nullptr, nullptr)->camera().setClearColor(0.05f, 0.06f, 0.09f, 1).setVirtualSize(640, 360);
    SceneCodec::load(SceneCodec::save(f.scene), f.scene);

    const auto* actor = dynamic_cast<CameraActor*>(f.scene.actor(1));
    REQUIRE(actor != nullptr);
    CHECK(actor->camera().clearColor().z == doctest::Approx(0.09f));
    CHECK(actor->camera().virtualSize().x == doctest::Approx(640));
    CHECK(actor->camera().virtualSize().y == doctest::Approx(360));
}

TEST_CASE("duplicate ids in a file do not abort the load") {
    Fixture f;
    SceneCodec::load(
        "version 6\n"
        "actors {\n"
        "    Actor {\n"
        "        id 5\n"
        "        name \"First\"\n"
        "        components {\n"
        "            SceneComponent {\n"
        "                id 5\n"
        "            }\n"
        "        }\n"
        "    }\n"
        "    Actor {\n"
        "        id 5\n"
        "        name \"Second\"\n"
        "    }\n"
        "}\n", f.scene);

    CHECK(f.scene.roots().size() == 2);
    CHECK(f.scene.find("Second") != nullptr);
    CHECK(f.scene.find("First")->components().size() == 1);
}

TEST_CASE("camera zoom beyond its range is clamped on load") {
    Fixture f;
    SceneCodec::load(
        "version 6\n"
        "actors {\n"
        "    CameraActor {\n"
        "        id 1\n"
        "        components {\n"
        "            CameraComponent {\n"
        "                id 2\n"
        "                zoom 900\n"
        "            }\n"
        "        }\n"
        "    }\n"
        "}\n", f.scene);

    CHECK(dynamic_cast<CameraComponent*>(f.scene.component(2))->zoom() == doctest::Approx(20));
}

TEST_CASE("attributes round trip with their types") {
    Fixture f;
    Actor* box = group(f.scene, 1, "Box", nullptr, 2);
    box->setAttribute("count", PropValue::integer(3));
    box->setAttribute("rate", PropValue::number(2.5));
    box->setAttribute("label", PropValue::text("crate"));
    box->setAttribute("solid", PropValue::flag(true));
    box->setAttribute("offset", PropValue::seq({PropValue::number(0), PropValue::number(2.5),
                                                PropValue::number(-1)}));

    SceneCodec::load(SceneCodec::save(f.scene), f.scene);
    const Actor* loaded = f.scene.actor(1);
    REQUIRE(loaded != nullptr);

    CHECK(loaded->attribute("count")->as<std::int64_t>() == 3);
    CHECK(loaded->attribute("rate")->as<double>() == doctest::Approx(2.5));
    CHECK(loaded->attribute("label")->as<std::string>() == "crate");
    CHECK(loaded->attribute("solid")->as<bool>());
    const PropSeq& offset = loaded->attribute("offset")->as<PropSeq>();
    REQUIRE(offset.size() == 3);
    CHECK(offset[1].as<double>() == doctest::Approx(2.5));
}

TEST_CASE("an attribute that is not a value is dropped on load") {
    Fixture f;
    SceneCodec::load(
        "version 6\n"
        "actors {\n"
        "    Actor {\n"
        "        id 1\n"
        "        attributes {\n"
        "            rate 2\n"
        "            nested {\n"
        "                deep 1\n"
        "            }\n"
        "        }\n"
        "    }\n"
        "}\n", f.scene);

    const Actor* box = f.scene.actor(1);
    REQUIRE(box != nullptr);
    CHECK(box->attribute("rate") != nullptr);
    CHECK(box->attribute("nested") == nullptr);
}

TEST_CASE("unknown actor and component classes are kept as placeholders and save back unchanged") {
    const std::string text = R"(version 6
actors {
    Turret {
        id 4
        name "Gun \"A\""
        enabled false
        range 12.5
        mode "burst"
        components {
            SceneComponent {
                id 5
                position 0 1 0
            }
            Laser {
                id 6
                position 1 2 3
                power 9
                attached {
                    SceneComponent {
                        id 7
                    }
                }
            }
        }
        attributes {
            team 2
        }
        children {
            Actor {
                id 9
            }
        }
    }
    Actor {
        id 8
    }
}
)";

    Fixture f;
    SceneCodec::load(text, f.scene);

    Actor* turret = f.scene.actor(4);
    REQUIRE(turret != nullptr);
    CHECK(f.types.nameOf(*turret) == "Turret");
    CHECK(dynamic_cast<cinder::Scene::ActorPlaceholder*>(turret) != nullptr);
    CHECK(turret->name() == "Gun \"A\"");
    CHECK(turret->isEnabled());
    REQUIRE(turret->components().size() == 3);
    CHECK(turret->rootComponent() == f.scene.component(5));
    auto* laser = dynamic_cast<cinder::Scene::ComponentPlaceholder*>(f.scene.component(6));
    REQUIRE(laser != nullptr);
    CHECK(f.types.nameOf(*laser) == "Laser");
    CHECK(dynamic_cast<SceneComponent*>(f.scene.component(7))->attachParent() == laser);
    REQUIRE(turret->attachedActors().size() == 1);
    CHECK(SceneCodec::save(f.scene) == text);

    Actor* copy = f.scene.clone(*turret, nullptr);
    REQUIRE(copy != nullptr);
    CHECK(f.types.nameOf(*copy) == "Turret");
    CHECK(copy->components().size() == 3);
    CHECK(copy->attachedActors().size() == 1);
}

TEST_CASE("an actor named with a lone brace does not open a block") {
    Fixture f;
    place<Actor>(f.scene, 3, "{", nullptr);
    place<Actor>(f.scene, 4, "after", nullptr);
    const std::string text = SceneCodec::save(f.scene);

    Fixture g;
    SceneCodec::load(text, g.scene);
    CHECK(g.scene.roots().size() == 2);
    REQUIRE(g.scene.byId(3) != nullptr);
    CHECK(g.scene.byId(3)->name() == "{");
    CHECK(SceneCodec::save(g.scene) == text);
}

TEST_CASE("unbalanced braces are rejected and leave the scene intact") {
    Fixture f;
    f.populate();
    CHECK_THROWS_AS(SceneCodec::load("version 6\nactors {\n    Actor {\n        id 1\n    }\n", f.scene), std::runtime_error);
    CHECK_THROWS_AS(SceneCodec::load("version 6\n}\nactors {\n}\n", f.scene), std::runtime_error);
    CHECK(f.scene.roots().size() == 2);
}

TEST_CASE("malformed numbers fall back instead of throwing or truncating") {
    Fixture f;
    SceneCodec::load("version 6\nactors {\n    Actor {\n        id 1e20\n        components {\n"
                     "            SceneComponent {\n                id 2\n                position 1 two 3x\n"
                     "            }\n        }\n    }\n}\n", f.scene);
    REQUIRE(f.scene.roots().size() == 1);
    const Actor* actor = f.scene.roots()[0];
    CHECK(actor->id() >= 1);
    REQUIRE(actor->transform() != nullptr);
    CHECK(actor->transform()->position().x == 1.0f);
    CHECK(actor->transform()->position().y == 0.0f);
    CHECK(actor->transform()->position().z == 0.0f);
}

TEST_CASE("a subnormal value survives a round trip") {
    Fixture f;
    Actor* tiny = group(f.scene, 2, "Tiny", nullptr, 3);
    tiny->transform()->setPosition(1.0e-40f, 0.0f, 0.0f);
    const std::string text = SceneCodec::save(f.scene);

    Fixture g;
    SceneCodec::load(text, g.scene);
    REQUIRE(g.scene.actor(2) != nullptr);
    CHECK(g.scene.actor(2)->transform()->position().x == 1.0e-40f);
}

TEST_CASE("a placeholder keeps the quoting of every token in a field") {
    const std::string text = R"(version 6
actors {
    Turret {
        id 4
        label 1 "a b" plain "c"
        components {
            Laser {
                id 5
                label "x y" z
            }
        }
    }
}
)";
    Fixture f;
    SceneCodec::load(text, f.scene);
    CHECK(SceneCodec::save(f.scene) == text);
}

TEST_CASE("a placeholder is cloned as a placeholder") {
    const std::string text = "version 6\nactors {\n    Turret {\n        id 4\n        range 3\n    }\n}\n";
    Fixture f;
    SceneCodec::load(text, f.scene);
    Actor* turret = f.scene.actor(4);
    REQUIRE(turret != nullptr);
    Actor* copy = f.scene.clone(*turret, nullptr);
    REQUIRE(copy != nullptr);
    CHECK(f.types.nameOf(*copy) == "Turret");
    CHECK(dynamic_cast<cinder::Scene::ActorPlaceholder*>(copy) != nullptr);
}

TEST_CASE("an out of range id gets a fresh one") {
    Fixture f;
    SceneCodec::load("version 6\nactors {\n    Actor {\n        id 2147483647\n    }\n}\n", f.scene);
    REQUIRE(f.scene.roots().size() == 1);
    CHECK(f.scene.roots()[0]->id() < Scene::MAX_ID);
}

TEST_CASE("a placeholder component is never the root") {
    Fixture f;
    SceneCodec::load("version 6\nactors {\n    Actor {\n        id 1\n        components {\n"
                     "            Laser {\n                id 2\n            }\n"
                     "            SceneComponent {\n                id 3\n            }\n        }\n    }\n}\n", f.scene);
    REQUIRE(f.scene.roots().size() == 1);
    CHECK(f.scene.roots()[0]->rootComponent() == f.scene.component(3));
}
