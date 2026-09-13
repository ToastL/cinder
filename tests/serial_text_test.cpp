#include <doctest/doctest.h>

#include "components/Builtins.hpp"
#include "components/Camera.hpp"
#include "components/SpriteRenderer.hpp"
#include "scene/Actor.hpp"
#include "scene/Components.hpp"
#include "scene/Scene.hpp"
#include "serial/SceneCodec.hpp"

#include <clocale>
#include <limits>
#include <string>

using cinder::components::Camera;
using cinder::components::SpriteRenderer;
using cinder::scene::Actor;
using cinder::scene::Components;
using cinder::scene::Scene;
using cinder::serial::SceneCodec;

namespace {

struct Tally : cinder::scene::Component {
    int count_ = 0;

    CINDER_COMPONENT(Tally, cinder::scene::Component) { CINDER_PROP(count_); }
};

struct Fixture {
    Components types;
    Scene scene{types};

    Fixture() {
        cinder::components::registerBuiltins(types);
        types.add<Tally>("Tally");
    }
};

bool contains(const std::string& text, std::string_view needle) {
    return text.find(needle) != std::string::npos;
}

}

TEST_CASE("integral floats lose the decimal point") {
    Fixture f;
    f.scene.spawn("A")->transform().setPosition(0, 3, 8);

    CHECK(contains(SceneCodec::save(f.scene), "position 0 3 8"));
}

TEST_CASE("fractional floats use the shortest round trip") {
    Fixture f;
    f.scene.spawn("A")->transform().setPosition(0.1f, 0, 0);

    CHECK(contains(SceneCodec::save(f.scene), "position 0.1 0 0"));
}

TEST_CASE("int props emit as integers") {
    Fixture f;
    f.scene.spawn("A")->add<Tally>()->count_ = 3;

    CHECK(contains(SceneCodec::save(f.scene), "count 3"));
}

TEST_CASE("asset paths emit quoted") {
    Fixture f;
    f.scene.spawn("A")->add<SpriteRenderer>()->setTexture("tiles/grass.png");

    CHECK(contains(SceneCodec::save(f.scene), "texture \"tiles/grass.png\""));
}

TEST_CASE("enums emit lowercase") {
    Fixture f;
    f.scene.spawn("A")->add<Camera>()->setProjection(Camera::Projection::Orthographic);

    CHECK(contains(SceneCodec::save(f.scene), "projection \"orthographic\""));
}

TEST_CASE("strings are quoted and escaped") {
    Fixture f;
    f.scene.spawn("a\"b\\c\td");

    const std::string text = SceneCodec::save(f.scene);
    CHECK(contains(text, "name \"a\\\"b\\\\c\\td\""));

    SceneCodec::load(text, f.scene);
    CHECK(f.scene.roots().front()->name() == "a\"b\\c\td");
}

TEST_CASE("non-finite floats become zero") {
    Fixture f;
    Actor* a = f.scene.spawn("A");
    a->transform().position() = glm::vec3(std::numeric_limits<float>::quiet_NaN(),
                                          std::numeric_limits<float>::infinity(), 1.0f);
    a->transform().dirty();

    CHECK(contains(SceneCodec::save(f.scene), "position 0 0 1"));
}

TEST_CASE("empty collections are omitted") {
    Fixture f;
    CHECK(SceneCodec::save(f.scene) == "version 3\n");
}

TEST_CASE("output is independent of the default locale") {
    Fixture f;
    Actor* a = f.scene.spawn("A");
    a->transform().setPosition(0.5f, 0, 0);
    a->add<Camera>()->setProjection(Camera::Projection::Orthographic);

    const char* before = std::setlocale(LC_ALL, nullptr);
    const std::string saved = before != nullptr ? before : "C";

    std::setlocale(LC_ALL, "tr_TR.UTF-8");
    const std::string text = SceneCodec::save(f.scene);
    std::setlocale(LC_ALL, saved.c_str());

    CHECK(contains(text, "position 0.5 0 0"));
    CHECK(contains(text, "projection \"orthographic\""));
}

TEST_CASE("nested blocks and indentation round trip") {
    Fixture f;
    Actor* root = f.scene.spawn(1, "Root", nullptr);
    Actor* mid = f.scene.spawn(2, "Mid", root);
    f.scene.spawn(3, "Leaf", mid);

    const std::string first = SceneCodec::save(f.scene);
    SceneCodec::load(first, f.scene);

    CHECK(SceneCodec::save(f.scene) == first);
    CHECK(f.scene.byId(3)->parent() == f.scene.byId(2));
    CHECK(f.scene.roots().size() == 1);
}
