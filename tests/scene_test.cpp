#include <doctest/doctest.h>

#include "components/Builtins.hpp"
#include "components/Group.hpp"
#include "components/Spin.hpp"
#include "scene/Attributes.hpp"
#include "scene/DrawList.hpp"
#include "scene/Node.hpp"
#include "scene/NodeTypes.hpp"
#include "scene/PropValue.hpp"
#include "scene/Scene.hpp"
#include "scene/Transform.hpp"

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using cinder::scene::DrawList;
using cinder::scene::Node;
using cinder::scene::NodeTypes;
using cinder::scene::PropValue;
using cinder::scene::Scene;

namespace {

struct Counter : Node {
    int starts = 0;
    int updates = 0;
    int renders = 0;
    int destroys = 0;

    void onStart() override { starts++; }
    void onUpdate(float dt) override { updates++; }
    void onRender(float alpha, DrawList& draws) override { renders++; }
    void onDestroy() override { destroys++; }

    CINDER_NODE(Counter, Node) {}
};

struct NullDraws : DrawList {
    int textureHandle(std::string_view path) override { return WHITE; }
    int meshHandle(std::string_view name) override { return 0; }
    void background(float r, float g, float b) override {}
    void sprite(int texture, const glm::mat4& model, glm::vec2 size, const glm::vec4& color) override {}
    void mesh(int mesh, int texture, const glm::mat4& model) override {}
    void camera(const cinder::scene::View& view) override {}
};

struct Suicide : Node {
    void onUpdate(float dt) override { destroy(); }

    CINDER_NODE(Suicide, Node) {}
};

struct Spawner : Node {
    Counter* spawned = nullptr;

    void onUpdate(float dt) override {
        if (spawned == nullptr) spawned = scene()->create<Counter>(this);
    }

    CINDER_NODE(Spawner, Node) {}
};

struct Recorder : cinder::scene::SceneObserver {
    std::vector<std::string> events;

    void attributeChanged(Node& node, const std::string& name) override { events.push_back("attribute " + name); }
    void childAdded(Node& parent, Node& child) override { events.push_back("added " + child.name()); }
    void childRemoved(Node& parent, Node& child) override { events.push_back("removed " + child.name()); }
    void destroying(Node& node) override { events.push_back("destroying " + node.name()); }
};

struct Fixture {
    NodeTypes types;
    Scene scene{types};
};

Counter* named(Scene& scene, const char* name, Node* parent = nullptr) {
    auto owned = std::make_unique<Counter>();
    owned->setName(name);
    return static_cast<Counter*>(scene.insert(std::move(owned), parent));
}

}

TEST_CASE("start runs once before the first update") {
    Fixture f;
    Counter* counter = f.scene.create<Counter>(nullptr);

    f.scene.update(0.1f);
    CHECK(counter->starts == 1);
    CHECK(counter->updates == 1);

    f.scene.update(0.1f);
    CHECK(counter->starts == 1);
    CHECK(counter->updates == 2);
}

TEST_CASE("nodes render before they start") {
    Fixture f;
    Counter* counter = f.scene.create<Counter>(nullptr);
    NullDraws draws;

    f.scene.render(0.0f, draws);
    CHECK(counter->starts == 0);
    CHECK(counter->renders == 1);
}

TEST_CASE("nodes added during update start on the next frame") {
    Fixture f;
    Spawner* spawner = f.scene.create<Spawner>(nullptr);

    f.scene.update(0.1f);
    CHECK(spawner->spawned->starts == 0);
    CHECK(spawner->spawned->updates == 0);

    f.scene.update(0.1f);
    CHECK(spawner->spawned->starts == 1);
    CHECK(spawner->spawned->updates == 1);
}

TEST_CASE("destroy is deferred to the end of the frame") {
    Fixture f;
    const int id = f.scene.create<Suicide>(nullptr)->id();
    Counter* sibling = f.scene.create<Counter>(nullptr);

    f.scene.update(0.1f);
    CHECK(sibling->updates == 1);
    CHECK(f.scene.byId(id) == nullptr);
}

TEST_CASE("destroying a parent destroys its children") {
    Fixture f;
    Node* parent = f.scene.create<Counter>(nullptr);
    Counter* child = f.scene.create<Counter>(parent);
    const int childId = child->id();

    f.scene.update(0.1f);
    CHECK(child->starts == 1);

    f.scene.destroy(parent);
    f.scene.update(0.1f);

    CHECK(f.scene.byId(childId) == nullptr);
    CHECK(f.scene.roots().empty());
}

TEST_CASE("destroyNow tears a subtree down at once") {
    Fixture f;
    Node* parent = f.scene.create<Counter>(nullptr);
    const int childId = f.scene.create<Counter>(parent)->id();
    Node* unstarted = f.scene.create<Counter>(nullptr);
    const int unstartedId = unstarted->id();

    f.scene.destroyNow(unstarted);
    f.scene.update(0.1f);
    f.scene.destroyNow(parent);

    CHECK(f.scene.byId(childId) == nullptr);
    CHECK(f.scene.byId(unstartedId) == nullptr);
    CHECK(f.scene.roots().empty());
}

TEST_CASE("disabled nodes start but neither update nor render") {
    Fixture f;
    Counter* counter = f.scene.create<Counter>(nullptr);
    counter->setEnabled(false);
    NullDraws draws;

    f.scene.update(0.1f);
    f.scene.render(0.0f, draws);
    CHECK(counter->starts == 1);
    CHECK(counter->updates == 0);
    CHECK(counter->renders == 0);
}

TEST_CASE("a disabled node skips its whole subtree") {
    Fixture f;
    Node* parent = f.scene.create<Counter>(nullptr);
    Counter* child = f.scene.create<Counter>(parent);
    parent->setEnabled(false);
    NullDraws draws;

    f.scene.update(0.1f);
    f.scene.render(0.0f, draws);
    CHECK(child->starts == 1);
    CHECK(child->updates == 0);
    CHECK(child->renders == 0);
    CHECK_FALSE(child->enabledInHierarchy());
}

TEST_CASE("ids are stable and lookups resolve") {
    Fixture f;
    Counter* a = named(f.scene, "a");
    Counter* b = named(f.scene, "b", a);

    CHECK(a->id() != b->id());
    CHECK(f.scene.byId(a->id()) == a);
    CHECK(f.scene.find("b") == b);
    CHECK(a->findFirstChild("b") == b);
    CHECK(f.scene.find("nope") == nullptr);
}

TEST_CASE("an explicit id is honoured and raises the watermark") {
    Fixture f;
    Node* loaded = f.scene.insert(std::make_unique<Counter>(), nullptr, 42);

    CHECK(loaded->id() == 42);
    CHECK(f.scene.byId(42) == loaded);
    CHECK(f.scene.create<Counter>(nullptr)->id() == 43);
}

TEST_CASE("a taken id is rejected") {
    Fixture f;
    const int id = f.scene.create<Counter>(nullptr)->id();
    CHECK_THROWS_AS(f.scene.insert(std::make_unique<Counter>(), nullptr, id), std::runtime_error);
}

TEST_CASE("an explicit id below the watermark does not lower it") {
    Fixture f;
    f.scene.insert(std::make_unique<Counter>(), nullptr, 10);
    f.scene.insert(std::make_unique<Counter>(), nullptr, 3);

    CHECK(f.scene.create<Counter>(nullptr)->id() == 11);
}

TEST_CASE("clear does not reissue ids") {
    Fixture f;
    const int before = f.scene.create<Counter>(nullptr)->id();

    f.scene.clear();
    Node* after = f.scene.create<Counter>(nullptr);

    CHECK(before != after->id());
    CHECK(f.scene.byId(before) == nullptr);
}

TEST_CASE("an unregistered node is named Node, a registered one after its class") {
    Fixture f;
    cinder::components::registerBuiltins(f.types);

    CHECK(f.scene.create<Counter>(nullptr)->name() == "Node");
    CHECK(f.scene.create("Group", nullptr)->name() == "Group");
    CHECK(f.scene.create("Nope", nullptr) == nullptr);
}

TEST_CASE("clone copies props, transform, attributes and children with fresh ids") {
    Fixture f;
    cinder::components::registerBuiltins(f.types);
    Node* box = f.scene.create("Group", nullptr);
    box->setName("Box");
    box->transform()->setPosition(1, 2, 3);
    box->setAttribute("hp", PropValue::integer(3));
    f.scene.create<cinder::components::Spin>(box)->setSpeed(0, 5, 0);

    Node* copy = f.scene.clone(*box, nullptr);
    REQUIRE(copy != nullptr);
    CHECK(copy->id() != box->id());
    CHECK(copy->name() == "Box");
    CHECK(copy->transform()->position().y == doctest::Approx(2));
    CHECK(copy->attribute("hp")->as<std::int64_t>() == 3);
    REQUIRE(copy->children().size() == 1);

    auto* spin = dynamic_cast<cinder::components::Spin*>(copy->children()[0]);
    REQUIRE(spin != nullptr);
    CHECK(spin != box->children()[0]);
    CHECK(spin->speed().y == doctest::Approx(5));
}

TEST_CASE("the observer hears structure, attribute and destroy events in order") {
    Fixture f;
    Recorder recorder;
    Counter* parent = named(f.scene, "parent");
    f.scene.setObserver(&recorder);

    Counter* child = named(f.scene, "child", parent);
    child->setAttribute("hp", PropValue::integer(1));
    child->setParent(nullptr);
    child->setParent(parent);
    f.scene.destroy(child);
    f.scene.update(0.1f);
    f.scene.setObserver(nullptr);

    const std::vector<std::string> expected{"added child", "attribute hp", "removed child",
                                            "added child", "removed child", "destroying child"};
    CHECK(recorder.events == expected);
}

TEST_CASE("an attribute notifies once per real change") {
    Fixture f;
    Recorder recorder;
    Node* a = f.scene.create<Counter>(nullptr);
    f.scene.setObserver(&recorder);

    a->setAttribute("hp", PropValue::integer(3));
    a->setAttribute("hp", PropValue::number(3.0));
    a->setAttribute("hp", PropValue::integer(4));
    a->removeAttribute("hp");
    a->removeAttribute("hp");
    f.scene.setObserver(nullptr);

    CHECK(recorder.events == std::vector<std::string>{"attribute hp", "attribute hp", "attribute hp"});
    CHECK(a->attribute("hp") == nullptr);
}

TEST_CASE("loaded attributes do not notify") {
    Fixture f;
    Recorder recorder;
    Node* a = f.scene.create<Counter>(nullptr);
    f.scene.setObserver(&recorder);

    a->loadAttributes({{"rate", PropValue::number(2.0)}});
    f.scene.setObserver(nullptr);

    CHECK(recorder.events.empty());
    CHECK(a->attribute("rate")->as<double>() == doctest::Approx(2.0));
}

TEST_CASE("attributes hold numbers, strings, bools and small vectors") {
    using cinder::scene::isAttributeName;
    using cinder::scene::isAttributeValue;

    CHECK(isAttributeName("max_speed2"));
    CHECK_FALSE(isAttributeName(""));
    CHECK_FALSE(isAttributeName("max speed"));

    CHECK(isAttributeValue(PropValue::integer(1)));
    CHECK(isAttributeValue(PropValue::text("a")));
    CHECK(isAttributeValue(PropValue::flag(true)));
    CHECK(isAttributeValue(PropValue::seq({PropValue::number(1), PropValue::integer(2)})));
    CHECK_FALSE(isAttributeValue(PropValue::seq({PropValue::number(1)})));
    CHECK_FALSE(isAttributeValue(PropValue::seq({PropValue::text("x"), PropValue::number(1)})));
    CHECK_FALSE(isAttributeValue(PropValue::rec({})));
}
