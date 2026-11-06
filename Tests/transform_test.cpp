#include <doctest/doctest.h>

#include "components/Folder.hpp"
#include "components/Group.hpp"
#include "scene/Node.hpp"
#include "scene/NodeTypes.hpp"
#include "scene/Scene.hpp"
#include "scene/Transform.hpp"

#include <glm/gtc/constants.hpp>

using cinder::scene::Node;
using cinder::scene::NodeTypes;
using cinder::scene::Scene;
using cinder::scene::Transform;

namespace {

struct Fixture {
    NodeTypes types;
    Scene scene{types};

    Node* spawn(const char* name, Node* parent = nullptr) {
        Node* node = scene.create<cinder::components::Group>(parent);
        node->setName(name);
        return node;
    }
};

void checkWorld(Node* node, float x, float y, float z) {
    const glm::vec3 p = node->transform()->worldPosition();
    CHECK(p.x == doctest::Approx(x).epsilon(1e-5));
    CHECK(p.y == doctest::Approx(y).epsilon(1e-5));
    CHECK(p.z == doctest::Approx(z).epsilon(1e-5));
}

}

TEST_CASE("world follows the parent") {
    Fixture f;
    Node* parent = f.spawn("parent");
    Node* child = f.spawn("child", parent);

    parent->transform()->setPosition(10, 0, 0);
    child->transform()->setPosition(0, 5, 0);
    checkWorld(child, 10, 5, 0);

    parent->transform()->setPosition(0, 0, 3);
    checkWorld(child, 0, 5, 3);
}

TEST_CASE("parent scale scales the child offset") {
    Fixture f;
    Node* parent = f.spawn("parent");
    Node* child = f.spawn("child", parent);

    parent->transform()->setScale(2);
    child->transform()->setPosition(0, 5, 0);
    checkWorld(child, 0, 10, 0);
}

TEST_CASE("parent rotation rotates the child offset") {
    Fixture f;
    Node* parent = f.spawn("parent");
    Node* child = f.spawn("child", parent);

    parent->transform()->setRotation(0, glm::half_pi<float>(), 0);
    child->transform()->setPosition(0, 0, 1);
    checkWorld(child, 1, 0, 0);
}

TEST_CASE("reparenting recomputes the world matrix") {
    Fixture f;
    Node* a = f.spawn("a");
    a->transform()->setPosition(10, 0, 0);
    Node* b = f.spawn("b");
    b->transform()->setPosition(-10, 0, 0);

    Node* child = f.spawn("child", a);
    child->transform()->setPosition(0, 1, 0);
    checkWorld(child, 10, 1, 0);

    child->setParent(b);
    checkWorld(child, -10, 1, 0);

    child->setParent(nullptr);
    checkWorld(child, 0, 1, 0);
}

TEST_CASE("three levels compose") {
    Fixture f;
    Node* a = f.spawn("a");
    Node* b = f.spawn("b", a);
    Node* c = f.spawn("c", b);

    a->transform()->setPosition(1, 0, 0);
    b->transform()->setPosition(0, 2, 0);
    c->transform()->setPosition(0, 0, 4);
    checkWorld(c, 1, 2, 4);

    a->transform()->setScale(2);
    checkWorld(c, 1, 4, 8);
}

TEST_CASE("in-place edits need an explicit dirty") {
    Fixture f;
    Node* e = f.spawn("e");
    checkWorld(e, 0, 0, 0);

    e->transform()->position() = glm::vec3(5, 0, 0);
    checkWorld(e, 0, 0, 0);

    e->transform()->dirty();
    checkWorld(e, 5, 0, 0);
}

TEST_CASE("writing position through a prop dirties the world matrix") {
    Fixture f;
    Node* e = f.spawn("e");
    checkWorld(e, 0, 0, 0);

    for (const cinder::reflect::PropDef& prop : cinder::reflect::props<Transform>()) {
        if (prop.name() != "position") continue;
        const float values[] = {5, 0, 0};
        prop.write(e->transform(), values);
    }
    checkWorld(e, 5, 0, 0);
}

TEST_CASE("reparenting into its own subtree is rejected") {
    Fixture f;
    Node* parent = f.spawn("parent");
    Node* child = f.spawn("child", parent);

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

TEST_CASE("a folder breaks the transform chain") {
    Fixture f;
    Node* group = f.spawn("group");
    group->transform()->setPosition(10, 0, 0);
    Node* folder = f.scene.create<cinder::components::Folder>(group);
    Node* child = f.spawn("child", folder);
    child->transform()->setPosition(0, 1, 0);

    checkWorld(child, 0, 1, 0);
}

TEST_CASE("rotate turns a node from where it already points") {
    Fixture f;
    Node* box = f.spawn("box");
    box->transform()->setRotation(0, 1, 0);
    box->transform()->rotate(0, 2, 0);

    CHECK(box->transform()->rotation().y == doctest::Approx(3.0f));
}

TEST_CASE("rotation is declared as an angle, so the editor shows it in degrees") {
    const cinder::reflect::PropList& defs = cinder::reflect::props<cinder::scene::Transform>();
    REQUIRE(defs.size() == 3);
    CHECK(defs[0].hint() == cinder::reflect::PropHint::None);
    CHECK(defs[1].name() == "rotation");
    CHECK(defs[1].hint() == cinder::reflect::PropHint::Angle);
}
