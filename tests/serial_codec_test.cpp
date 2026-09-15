#include <doctest/doctest.h>

#include "components/Builtins.hpp"
#include "components/Camera.hpp"
#include "components/Group.hpp"
#include "components/MeshPart.hpp"
#include "components/Sprite.hpp"
#include "scene/Node.hpp"
#include "scene/NodeTypes.hpp"
#include "scene/PropValue.hpp"
#include "scene/Scene.hpp"
#include "serial/SceneCodec.hpp"

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>

using cinder::components::Camera;
using cinder::components::Group;
using cinder::components::MeshPart;
using cinder::components::Sprite;
using cinder::scene::Node;
using cinder::scene::NodeTypes;
using cinder::scene::PropSeq;
using cinder::scene::PropValue;
using cinder::scene::Scene;
using cinder::serial::SceneCodec;

namespace {

template <class T>
T* place(Scene& scene, int id, const char* name, Node* parent) {
    auto owned = std::make_unique<T>();
    if (name != nullptr) owned->setName(name);
    return static_cast<T*>(scene.insert(std::move(owned), parent, id));
}

struct Fixture {
    NodeTypes types;
    Scene scene{types};

    Fixture() { cinder::components::registerBuiltins(types); }

    void populate() {
        Camera* camera = place<Camera>(scene, 7, nullptr, nullptr);
        camera->transform()->setPosition(0, 3, 8);
        camera->setFov(70);

        Sprite* player = place<Sprite>(scene, 9, "Play\"er", nullptr);
        player->transform()->setPosition(-1.5f, 0, 0.25f);
        player->setSize(64, 48);
        player->setColor(1, 0.2f, 0.2f, 1);
        player->setAttribute("hp", PropValue::integer(3));
        player->setAttribute("tag", PropValue::text("hero"));

        Group* muzzle = place<Group>(scene, 12, "Muzzle", player);
        muzzle->transform()->setPosition(0, 1, 0);
        muzzle->setEnabled(false);
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
        "version 4\n"
        "nodes {\n"
        "    Camera {\n"
        "        id 7\n"
        "        position 0 3 8\n"
        "        fov 70\n"
        "    }\n"
        "    Sprite {\n"
        "        id 9\n"
        "        name \"Play\\\"er\"\n"
        "        position -1.5 0 0.25\n"
        "        size 64 48\n"
        "        color 1 0.2 0.2 1\n"
        "        attributes {\n"
        "            hp 3\n"
        "            tag \"hero\"\n"
        "        }\n"
        "        children {\n"
        "            Group {\n"
        "                id 12\n"
        "                name \"Muzzle\"\n"
        "                position 0 1 0\n"
        "                enabled false\n"
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
    f.scene.create<Sprite>(nullptr);

    const std::string text = SceneCodec::save(f.scene);

    CHECK_FALSE(contains(text, "enabled"));
    CHECK_FALSE(contains(text, "scale"));
    CHECK_FALSE(contains(text, "rotation"));
    CHECK_FALSE(contains(text, "texture"));
    CHECK_FALSE(contains(text, "size"));
    CHECK_FALSE(contains(text, "name"));
    CHECK(contains(text, "Sprite {"));
}

TEST_CASE("non-default props survive") {
    Fixture f;
    f.populate();
    SceneCodec::load(SceneCodec::save(f.scene), f.scene);

    CHECK(dynamic_cast<Camera*>(f.scene.byId(7))->fov() == doctest::Approx(70));

    const Sprite* sprite = dynamic_cast<Sprite*>(f.scene.byId(9));
    REQUIRE(sprite != nullptr);
    CHECK(sprite->size().x == doctest::Approx(64));
    CHECK(sprite->color().y == doctest::Approx(0.2f));
}

TEST_CASE("ids, names, classes and the hierarchy survive") {
    Fixture f;
    f.populate();
    SceneCodec::load(SceneCodec::save(f.scene), f.scene);

    CHECK(f.scene.byId(7)->name() == "Camera");
    CHECK(f.scene.byId(9)->name() == "Play\"er");
    CHECK(f.scene.byId(3) == nullptr);
    CHECK(dynamic_cast<Group*>(f.scene.byId(12)) != nullptr);
    CHECK(f.scene.byId(12)->parent() == f.scene.byId(9));
    CHECK_FALSE(f.scene.byId(12)->isEnabled());
    CHECK(f.scene.roots().size() == 2);
}

TEST_CASE("an unknown class is skipped with its subtree") {
    Fixture f;
    SceneCodec::load(
        "version 4\n"
        "nodes {\n"
        "    Nope {\n"
        "        id 4\n"
        "        children {\n"
        "            Group {\n"
        "                id 5\n"
        "            }\n"
        "        }\n"
        "    }\n"
        "    Group {\n"
        "        id 6\n"
        "    }\n"
        "}\n", f.scene);

    CHECK(f.scene.byId(4) == nullptr);
    CHECK(f.scene.byId(5) == nullptr);
    CHECK(f.scene.byId(6) != nullptr);
}

TEST_CASE("load replaces the previous scene") {
    Fixture f;
    f.populate();
    SceneCodec::load("version 4\nnodes {\n    Group {\n        id 1\n    }\n}\n", f.scene);

    CHECK(f.scene.roots().size() == 1);
    CHECK(f.scene.byId(7) == nullptr);
}

TEST_CASE("a newer version is rejected and leaves the scene intact") {
    Fixture f;
    f.populate();

    CHECK_THROWS_AS(SceneCodec::load("version 99\nnodes {\n}\n", f.scene), std::runtime_error);
    CHECK(f.scene.roots().size() == 2);
}

TEST_CASE("a missing version is rejected") {
    Fixture f;
    CHECK_THROWS_AS(SceneCodec::load("nodes {\n}\n", f.scene), std::runtime_error);
}

TEST_CASE("actor-era files are rejected") {
    Fixture f;
    CHECK_THROWS_AS(SceneCodec::load("version 3\nactors {\n}\n", f.scene), std::runtime_error);
    CHECK_THROWS_AS(SceneCodec::load("version 1\nactors {\n}\n", f.scene), std::runtime_error);
}

TEST_CASE("asset references survive by name") {
    Fixture f;
    place<MeshPart>(f.scene, 1, "Box", nullptr)->setMesh("sphere").setTexture("crate.png");
    place<Sprite>(f.scene, 2, "Tile", nullptr)->setTexture("tile.png");
    SceneCodec::load(SceneCodec::save(f.scene), f.scene);

    CHECK(dynamic_cast<MeshPart*>(f.scene.byId(1))->mesh() == "sphere");
    CHECK(dynamic_cast<MeshPart*>(f.scene.byId(1))->texture() == "crate.png");
    CHECK(dynamic_cast<Sprite*>(f.scene.byId(2))->texture() == "tile.png");
}

TEST_CASE("camera render settings survive") {
    Fixture f;
    place<Camera>(f.scene, 1, nullptr, nullptr)->setClearColor(0.05f, 0.06f, 0.09f, 1).setVirtualSize(640, 360);
    SceneCodec::load(SceneCodec::save(f.scene), f.scene);

    const Camera* camera = dynamic_cast<Camera*>(f.scene.byId(1));
    REQUIRE(camera != nullptr);
    CHECK(camera->clearColor().z == doctest::Approx(0.09f));
    CHECK(camera->virtualSize().x == doctest::Approx(640));
    CHECK(camera->virtualSize().y == doctest::Approx(360));
}

TEST_CASE("duplicate ids in a file do not abort the load") {
    Fixture f;
    SceneCodec::load(
        "version 4\n"
        "nodes {\n"
        "    Group {\n"
        "        id 5\n"
        "        name \"First\"\n"
        "    }\n"
        "    Group {\n"
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
        "version 4\n"
        "nodes {\n"
        "    Camera {\n"
        "        id 1\n"
        "        zoom 900\n"
        "    }\n"
        "}\n", f.scene);

    CHECK(dynamic_cast<Camera*>(f.scene.byId(1))->zoom() == doctest::Approx(20));
}

TEST_CASE("attributes round trip with their types") {
    Fixture f;
    Group* box = place<Group>(f.scene, 1, "Box", nullptr);
    box->setAttribute("count", PropValue::integer(3));
    box->setAttribute("rate", PropValue::number(2.5));
    box->setAttribute("label", PropValue::text("crate"));
    box->setAttribute("solid", PropValue::flag(true));
    box->setAttribute("offset", PropValue::seq({PropValue::number(0), PropValue::number(2.5),
                                                PropValue::number(-1)}));

    SceneCodec::load(SceneCodec::save(f.scene), f.scene);
    const Node* loaded = f.scene.byId(1);
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
        "version 4\n"
        "nodes {\n"
        "    Group {\n"
        "        id 1\n"
        "        attributes {\n"
        "            rate 2\n"
        "            nested {\n"
        "                deep 1\n"
        "            }\n"
        "        }\n"
        "    }\n"
        "}\n", f.scene);

    const Node* box = f.scene.byId(1);
    REQUIRE(box != nullptr);
    CHECK(box->attribute("rate") != nullptr);
    CHECK(box->attribute("nested") == nullptr);
}
