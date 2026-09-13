#include <doctest/doctest.h>

#include "components/Builtins.hpp"
#include "components/Camera.hpp"
#include "components/MeshRenderer.hpp"
#include "components/SpriteRenderer.hpp"
#include "scene/Actor.hpp"
#include "scene/Components.hpp"
#include "scene/PropValue.hpp"
#include "scene/Scene.hpp"
#include "script/Script.hpp"
#include "serial/SceneCodec.hpp"

#include <lua.hpp>

#include <cstdint>
#include <memory>
#include <string>

using cinder::components::Camera;
using cinder::components::MeshRenderer;
using cinder::components::SpriteRenderer;
using cinder::scene::Actor;
using cinder::scene::Components;
using cinder::scene::PropSeq;
using cinder::scene::PropValue;
using cinder::scene::Scene;
using cinder::script::Script;
using cinder::serial::SceneCodec;

namespace {

struct Fixture {
    Components types;
    Scene scene{types};

    Fixture() { cinder::components::registerBuiltins(types); }

    void populate() {
        Actor* camera = scene.spawn(7, "Camera", nullptr);
        camera->transform().setPosition(0, 3, 8);
        camera->add<Camera>()->setFov(70);

        Actor* player = scene.spawn(9, "Play\"er", nullptr);
        player->transform().setPosition(-1.5f, 0, 0.25f);
        SpriteRenderer* sprite = player->add<SpriteRenderer>();
        sprite->setSize(64, 48);
        sprite->setColor(1, 0.2f, 0.2f, 1);
        player->setAttribute("hp", PropValue::integer(3));
        player->setAttribute("tag", PropValue::text("hero"));

        Actor* muzzle = scene.spawn(12, "Muzzle", player);
        muzzle->transform().setPosition(0, 1, 0);
        muzzle->setActive(false);
    }
};

bool contains(const std::string& text, std::string_view needle) {
    return text.find(needle) != std::string::npos;
}

}

TEST_CASE("the emitted format is pinned byte for byte") {
    Fixture f;
    f.populate();

    const std::string golden =
        "version 3\n"
        "actors {\n"
        "    actor {\n"
        "        id 7\n"
        "        name \"Camera\"\n"
        "        position 0 3 8\n"
        "        components {\n"
        "            Camera {\n"
        "                fov 70\n"
        "            }\n"
        "        }\n"
        "    }\n"
        "    actor {\n"
        "        id 9\n"
        "        name \"Play\\\"er\"\n"
        "        position -1.5 0 0.25\n"
        "        attributes {\n"
        "            hp 3\n"
        "            tag \"hero\"\n"
        "        }\n"
        "        components {\n"
        "            SpriteRenderer {\n"
        "                size 64 48\n"
        "                color 1 0.2 0.2 1\n"
        "            }\n"
        "        }\n"
        "        children {\n"
        "            actor {\n"
        "                id 12\n"
        "                name \"Muzzle\"\n"
        "                active false\n"
        "                position 0 1 0\n"
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
    const std::string second = SceneCodec::save(f.scene);

    CHECK(first == second);
}

TEST_CASE("default valued props are omitted") {
    Fixture f;
    f.scene.spawn("Bare")->add<SpriteRenderer>();

    const std::string text = SceneCodec::save(f.scene);

    CHECK_FALSE(contains(text, "enabled"));
    CHECK_FALSE(contains(text, "scale"));
    CHECK_FALSE(contains(text, "rotation"));
    CHECK_FALSE(contains(text, "texture"));
    CHECK_FALSE(contains(text, "size"));
    CHECK(contains(text, "SpriteRenderer"));
}

TEST_CASE("non-default props survive") {
    Fixture f;
    f.populate();
    SceneCodec::load(SceneCodec::save(f.scene), f.scene);

    CHECK(f.scene.byId(7)->get<Camera>()->fov() == doctest::Approx(70));

    SpriteRenderer* sprite = f.scene.byId(9)->get<SpriteRenderer>();
    CHECK(sprite->size().x == doctest::Approx(64));
    CHECK(sprite->color().y == doctest::Approx(0.2f));
}

TEST_CASE("actor ids and names survive") {
    Fixture f;
    f.populate();
    SceneCodec::load(SceneCodec::save(f.scene), f.scene);

    CHECK(f.scene.byId(7)->name() == "Camera");
    CHECK(f.scene.byId(9)->name() == "Play\"er");
    CHECK(f.scene.byId(3) == nullptr);
}

TEST_CASE("hierarchy and active flag survive") {
    Fixture f;
    f.populate();
    SceneCodec::load(SceneCodec::save(f.scene), f.scene);

    CHECK(f.scene.roots().size() == 2);
    Actor* muzzle = f.scene.byId(12);
    CHECK(muzzle->parent() == f.scene.byId(9));
    CHECK_FALSE(muzzle->activeSelf());
    CHECK(f.scene.byId(9)->activeSelf());
}

TEST_CASE("loaded transforms are dirty") {
    Fixture f;
    f.populate();
    SceneCodec::load(SceneCodec::save(f.scene), f.scene);

    CHECK(f.scene.byId(7)->transform().world()[3][2] == doctest::Approx(8));
}

TEST_CASE("saving does not write back through PropDef") {
    Fixture f;
    Actor* a = f.scene.spawn("Camera");
    a->add<Camera>()->setZoom(50);

    SceneCodec::save(f.scene);

    CHECK(a->get<Camera>()->zoom() == doctest::Approx(50));
}

TEST_CASE("loading replaces the existing scene") {
    Fixture f;
    f.populate();
    const std::string text = SceneCodec::save(f.scene);

    f.scene.spawn("Extra");
    SceneCodec::load(text, f.scene);

    CHECK(f.scene.roots().size() == 2);
    CHECK(f.scene.find("Extra") == nullptr);
}

TEST_CASE("components come back attached") {
    Fixture f;
    f.populate();
    SceneCodec::load(SceneCodec::save(f.scene), f.scene);

    CHECK(f.scene.byId(7)->get<Camera>() != nullptr);
}

TEST_CASE("an unknown component type is skipped") {
    Fixture f;
    SceneCodec::load(
        "version 2\n"
        "actors {\n"
        "    actor {\n"
        "        id 4\n"
        "        name \"Ghost\"\n"
        "        components {\n"
        "            Nope {\n"
        "                whatever 1\n"
        "            }\n"
        "        }\n"
        "    }\n"
        "}\n", f.scene);

    REQUIRE(f.scene.byId(4) != nullptr);
    CHECK(f.scene.byId(4)->components().empty());
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

TEST_CASE("a version 1 file is rejected") {
    Fixture f;
    CHECK_THROWS_AS(SceneCodec::load("version 1\nactors {\n}\n", f.scene), std::runtime_error);
}

TEST_CASE("asset references survive by name") {
    Fixture f;
    f.scene.spawn(1, "Box", nullptr)->add<MeshRenderer>()->setMesh("sphere").setTexture("crate.png");
    f.scene.spawn(2, "Tile", nullptr)->add<SpriteRenderer>()->setTexture("tile.png");
    SceneCodec::load(SceneCodec::save(f.scene), f.scene);

    CHECK(f.scene.byId(1)->get<MeshRenderer>()->mesh() == "sphere");
    CHECK(f.scene.byId(1)->get<MeshRenderer>()->texture() == "crate.png");
    CHECK(f.scene.byId(2)->get<SpriteRenderer>()->texture() == "tile.png");
}

TEST_CASE("camera render settings survive") {
    Fixture f;
    f.scene.spawn(1, "Camera", nullptr)->add<Camera>()->setClearColor(0.05f, 0.06f, 0.09f, 1)
            .setVirtualSize(640, 360);
    SceneCodec::load(SceneCodec::save(f.scene), f.scene);

    const Camera* camera = f.scene.byId(1)->get<Camera>();
    CHECK(camera->clearColor().z == doctest::Approx(0.09f));
    CHECK(camera->virtualSize().x == doctest::Approx(640));
    CHECK(camera->virtualSize().y == doctest::Approx(360));
}

TEST_CASE("duplicate ids in a file do not abort the load") {
    Fixture f;
    SceneCodec::load(
        "version 2\n"
        "actors {\n"
        "    actor {\n"
        "        id 5\n"
        "        name \"First\"\n"
        "    }\n"
        "    actor {\n"
        "        id 5\n"
        "        name \"Second\"\n"
        "    }\n"
        "}\n", f.scene);

    CHECK(f.scene.roots().size() == 2);
    CHECK(f.scene.find("Second") != nullptr);
}

TEST_CASE("camera zoom beyond its range is clamped on load") {
    Fixture f;
    SceneCodec::load(
        "version 2\n"
        "actors {\n"
        "    actor {\n"
        "        id 1\n"
        "        name \"Camera\"\n"
        "        components {\n"
        "            Camera {\n"
        "                zoom 900\n"
        "            }\n"
        "        }\n"
        "    }\n"
        "}\n", f.scene);

    CHECK(f.scene.byId(1)->get<Camera>()->zoom() == doctest::Approx(20));
}

TEST_CASE("attributes round trip with their types") {
    Fixture f;
    Actor* box = f.scene.spawn(1, "Box", nullptr);
    box->setAttribute("count", PropValue::integer(3));
    box->setAttribute("rate", PropValue::number(2.5));
    box->setAttribute("label", PropValue::text("crate"));
    box->setAttribute("solid", PropValue::flag(true));
    box->setAttribute("offset", PropValue::seq({PropValue::number(0), PropValue::number(2.5),
                                                PropValue::number(-1)}));

    SceneCodec::load(SceneCodec::save(f.scene), f.scene);
    const Actor* loaded = f.scene.byId(1);
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
        "version 3\n"
        "actors {\n"
        "    actor {\n"
        "        id 1\n"
        "        name \"Box\"\n"
        "        attributes {\n"
        "            rate 2\n"
        "            nested {\n"
        "                deep 1\n"
        "            }\n"
        "        }\n"
        "    }\n"
        "}\n", f.scene);

    const Actor* box = f.scene.byId(1);
    REQUIRE(box != nullptr);
    CHECK(box->attribute("rate") != nullptr);
    CHECK(box->attribute("nested") == nullptr);
}

TEST_CASE("a version 2 Behaviour loads as a Script with its data as attributes") {
    lua_State* state = luaL_newstate();
    {
        Fixture f;
        f.types.add<Script>("Script", [state] { return std::make_unique<Script>(state); });
        SceneCodec::load(
            "version 2\n"
            "actors {\n"
            "    actor {\n"
            "        id 3\n"
            "        name \"Box\"\n"
            "        components {\n"
            "            Behaviour {\n"
            "                enabled false\n"
            "                script \"scripts/bobber.lua\"\n"
            "                data {\n"
            "                    phase -8\n"
            "                }\n"
            "            }\n"
            "        }\n"
            "    }\n"
            "}\n", f.scene);

        const Actor* box = f.scene.byId(3);
        REQUIRE(box != nullptr);
        const Script* script = box->get<Script>();
        REQUIRE(script != nullptr);
        CHECK(script->file() == "scripts/bobber.lua");
        CHECK_FALSE(script->isEnabled());
        CHECK(box->attribute("phase")->as<std::int64_t>() == -8);

        const std::string text = SceneCodec::save(f.scene);
        CHECK(contains(text, "version 3"));
        CHECK(contains(text, "Script {"));
        CHECK_FALSE(contains(text, "Behaviour"));
    }
    lua_close(state);
}
