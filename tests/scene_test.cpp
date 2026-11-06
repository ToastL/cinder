#include <doctest/doctest.h>

#include "components/Builtins.hpp"
#include "components/Group.hpp"
#include "components/Sprite.hpp"
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

struct Recorder {
    std::vector<std::string> events;

    void watch(Scene& scene, Node& source, Node& owner) {
        using cinder::scene::Signal;
        using cinder::scene::SignalArgs;
        scene.connect(source, Signal::AttributeChanged, owner,
                      [this](const SignalArgs& args) { events.push_back("attribute " + std::string(args.name)); });
        scene.connect(source, Signal::ChildAdded, owner,
                      [this](const SignalArgs& args) { events.push_back("added " + args.other->name()); });
        scene.connect(source, Signal::ChildRemoved, owner,
                      [this](const SignalArgs& args) { events.push_back("removed " + args.other->name()); });
        scene.connect(source, Signal::Destroying, owner,
                      [this, name = source.name()](const SignalArgs&) { events.push_back("destroying " + name); });
    }
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
    f.scene.create<cinder::components::Sprite>(box)->setSize(5, 7);

    Node* copy = f.scene.clone(*box, nullptr);
    REQUIRE(copy != nullptr);
    CHECK(copy->id() != box->id());
    CHECK(copy->name() == "Box");
    CHECK(copy->transform()->position().y == doctest::Approx(2));
    CHECK(copy->attribute("hp")->as<std::int64_t>() == 3);
    REQUIRE(copy->children().size() == 1);

    auto* sprite = dynamic_cast<cinder::components::Sprite*>(copy->children()[0]);
    REQUIRE(sprite != nullptr);
    CHECK(sprite != box->children()[0]);
    CHECK(sprite->size().y == doctest::Approx(7));
}

TEST_CASE("signals report structure, attribute and destroy events in order") {
    Fixture f;
    Recorder recorder;
    Counter* parent = named(f.scene, "parent");
    Counter* ear = named(f.scene, "ear");
    f.scene.update(0.0f);
    recorder.watch(f.scene, *parent, *ear);

    Counter* child = named(f.scene, "child", parent);
    recorder.watch(f.scene, *child, *ear);
    child->setAttribute("hp", PropValue::integer(1));
    child->setParent(nullptr);
    child->setParent(parent);
    f.scene.destroy(child);
    f.scene.update(0.1f);

    const std::vector<std::string> expected{"added child", "attribute hp", "removed child",
                                            "added child", "removed child", "destroying child"};
    CHECK(recorder.events == expected);
}

TEST_CASE("an attribute notifies once per real change") {
    Fixture f;
    Recorder recorder;
    Node* a = f.scene.create<Counter>(nullptr);
    f.scene.update(0.0f);
    recorder.watch(f.scene, *a, *a);

    a->setAttribute("hp", PropValue::integer(3));
    a->setAttribute("hp", PropValue::number(3.0));
    a->setAttribute("hp", PropValue::integer(4));
    a->removeAttribute("hp");
    a->removeAttribute("hp");

    CHECK(recorder.events == std::vector<std::string>{"attribute hp", "attribute hp", "attribute hp"});
    CHECK(a->attribute("hp") == nullptr);
}

TEST_CASE("loaded attributes do not notify") {
    Fixture f;
    Recorder recorder;
    Node* a = f.scene.create<Counter>(nullptr);
    f.scene.update(0.0f);
    recorder.watch(f.scene, *a, *a);

    a->loadAttributes({{"rate", PropValue::number(2.0)}});

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

namespace {

struct Listener : Node {
    std::vector<std::string> attributes;
    std::vector<int> added;
    std::vector<int> removed;

    void onAttributeChanged(std::string_view name) override { attributes.emplace_back(name); }
    void onChildAdded(Node& child) override { added.push_back(child.id()); }
    void onChildRemoved(Node& child) override { removed.push_back(child.id()); }

    CINDER_NODE(Listener, Node) {}
};

}

TEST_CASE("event hooks reach only started nodes, so nothing runs in Edit mode") {
    NodeTypes types;
    Scene scene(types);
    auto* listener = scene.create<Listener>(nullptr);

    listener->setAttribute("hp", PropValue::integer(3));
    scene.create<Counter>(listener);
    CHECK(listener->attributes.empty());
    CHECK(listener->added.empty());

    scene.update(0.0f);
    listener->setAttribute("hp", PropValue::integer(4));
    const int child = scene.create<Counter>(listener)->id();
    scene.destroyNow(scene.byId(child));

    CHECK(listener->attributes == std::vector<std::string>{"hp"});
    CHECK(listener->added == std::vector<int>{child});
    CHECK(listener->removed == std::vector<int>{child});
}

TEST_CASE("a connection hears another node's signal and dies with either node") {
    NodeTypes types;
    Scene scene(types);
    auto* source = scene.create<Counter>(nullptr);
    auto* owner = scene.create<Counter>(source);
    std::vector<std::string> heard;
    scene.connect(*source, cinder::scene::Signal::AttributeChanged, *owner,
                  [&heard](const cinder::scene::SignalArgs& args) { heard.emplace_back(args.name); });

    source->setAttribute("before", PropValue::integer(1));
    scene.update(0.0f);
    source->setAttribute("speed", PropValue::integer(2));
    CHECK(heard == std::vector<std::string>{"speed"});

    scene.destroyNow(owner);
    source->setAttribute("after", PropValue::integer(3));
    CHECK(heard.size() == 1);

    auto* other = scene.create<Counter>(nullptr);
    scene.update(0.0f);
    int destroying = 0;
    scene.connect(*source, cinder::scene::Signal::Destroying, *other,
                  [&destroying](const cinder::scene::SignalArgs&) { destroying++; });
    const int handle = scene.connect(*source, cinder::scene::Signal::AttributeChanged, *other,
                                     [&heard](const cinder::scene::SignalArgs&) { heard.emplace_back("x"); });
    scene.disconnect(handle);
    source->setAttribute("quiet", PropValue::integer(4));
    CHECK(heard.size() == 1);
    scene.destroyNow(source);
    CHECK(destroying == 1);
}

TEST_CASE("timers fire on scene time, repeat, and are owned by a node") {
    NodeTypes types;
    Scene scene(types);
    auto* owner = scene.create<Counter>(nullptr);
    int once = 0;
    int repeated = 0;
    scene.after(*owner, 0.5, [&once] { once++; });
    const int every = scene.every(*owner, 0.25, [&repeated] { repeated++; });

    for (int i = 0; i < 4; ++i) scene.update(0.125f);
    CHECK(once == 1);
    CHECK(repeated == 2);
    CHECK(scene.time() == doctest::Approx(0.5));

    scene.cancel(every);
    for (int i = 0; i < 4; ++i) scene.update(0.125f);
    CHECK(repeated == 2);

    int orphaned = 0;
    scene.after(*owner, 0.1, [&orphaned] { orphaned++; });
    scene.destroy(owner);
    for (int i = 0; i < 4; ++i) scene.update(0.125f);
    CHECK(orphaned == 0);

    auto* survivor = scene.create<Counter>(nullptr);
    scene.after(*survivor, 1.0, [&orphaned] { orphaned++; });
    scene.clear();
    CHECK(scene.time() == 0.0);
}

namespace {

struct First final : Node {
    CINDER_NODE(First, Node) {}
};

struct Second final : Node {
    CINDER_NODE(Second, Node) {}
};

}

TEST_CASE("a removed class keeps its slot and takes a different type when added again") {
    NodeTypes types;
    types.add<Counter>("Before");
    types.add<First>("Swapped");
    types.add<Counter>("After");
    types.remove("Swapped");
    CHECK(types.create("Swapped") == nullptr);
    CHECK(types.fallback("Swapped") == nullptr);
    CHECK(types.nameOf(First{}).empty());

    types.add<Second>("Swapped");
    CHECK(types.nameOf(*types.create("Swapped")) == "Swapped");
    CHECK(types.nameOf(Second{}) == "Swapped");
    REQUIRE(types.registered().size() == 3);
    CHECK(types.registered()[1].name == "Swapped");
}
