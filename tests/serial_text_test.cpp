#include <doctest/doctest.h>

#include "components/Builtins.hpp"
#include "components/Camera.hpp"
#include "components/Group.hpp"
#include "components/Sprite.hpp"
#include "scene/Node.hpp"
#include "scene/NodeTypes.hpp"
#include "scene/Scene.hpp"
#include "serial/SceneCodec.hpp"

#include <clocale>
#include <limits>
#include <memory>
#include <string>

using cinder::components::Camera;
using cinder::components::Group;
using cinder::components::Sprite;
using cinder::scene::Node;
using cinder::scene::NodeTypes;
using cinder::scene::Scene;
using cinder::serial::SceneCodec;

namespace {

struct Tally : cinder::scene::Node {
    int count_ = 0;

    CINDER_NODE(Tally, cinder::scene::Node) { CINDER_PROP(count_); }
};

struct Fixture {
    NodeTypes types;
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
    f.scene.create<Group>(nullptr)->transform()->setPosition(0, 3, 8);

    CHECK(contains(SceneCodec::save(f.scene), "position 0 3 8"));
}

TEST_CASE("fractional floats use the shortest round trip") {
    Fixture f;
    f.scene.create<Group>(nullptr)->transform()->setPosition(0.1f, 0, 0);

    CHECK(contains(SceneCodec::save(f.scene), "position 0.1 0 0"));
}

TEST_CASE("int props emit as integers") {
    Fixture f;
    f.scene.create<Tally>(nullptr)->count_ = 3;

    CHECK(contains(SceneCodec::save(f.scene), "count 3"));
}

TEST_CASE("asset paths emit quoted") {
    Fixture f;
    f.scene.create<Sprite>(nullptr)->setTexture("tiles/grass.png");

    CHECK(contains(SceneCodec::save(f.scene), "texture \"tiles/grass.png\""));
}

TEST_CASE("enums emit lowercase") {
    Fixture f;
    f.scene.create<Camera>(nullptr)->setProjection(Camera::Projection::Orthographic);

    CHECK(contains(SceneCodec::save(f.scene), "projection \"orthographic\""));
}

TEST_CASE("strings are quoted and escaped") {
    Fixture f;
    f.scene.create<Group>(nullptr)->setName("a\"b\\c\td");

    const std::string text = SceneCodec::save(f.scene);
    CHECK(contains(text, "name \"a\\\"b\\\\c\\td\""));

    SceneCodec::load(text, f.scene);
    CHECK(f.scene.roots().front()->name() == "a\"b\\c\td");
}

TEST_CASE("non-finite floats become zero") {
    Fixture f;
    Node* a = f.scene.create<Group>(nullptr);
    a->transform()->position() = glm::vec3(std::numeric_limits<float>::quiet_NaN(),
                                           std::numeric_limits<float>::infinity(), 1.0f);
    a->transform()->dirty();

    CHECK(contains(SceneCodec::save(f.scene), "position 0 0 1"));
}

TEST_CASE("empty collections are omitted") {
    Fixture f;
    CHECK(SceneCodec::save(f.scene) == "version 4\n");
}

TEST_CASE("output is independent of the default locale") {
    Fixture f;
    Camera* camera = f.scene.create<Camera>(nullptr);
    camera->transform()->setPosition(0.5f, 0, 0);
    camera->setProjection(Camera::Projection::Orthographic);

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
    Node* root = f.scene.insert(std::make_unique<Group>(), nullptr, 1);
    Node* mid = f.scene.insert(std::make_unique<Group>(), root, 2);
    f.scene.insert(std::make_unique<Group>(), mid, 3);

    const std::string first = SceneCodec::save(f.scene);
    SceneCodec::load(first, f.scene);

    CHECK(SceneCodec::save(f.scene) == first);
    CHECK(f.scene.byId(3)->parent() == f.scene.byId(2));
    CHECK(f.scene.roots().size() == 1);
}
