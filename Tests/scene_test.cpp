#include <doctest/doctest.h>

#include "Components/Actors.hpp"
#include "Components/Builtins.hpp"
#include "Components/SpriteComponent.hpp"
#include "Scene/Actor.hpp"
#include "Scene/Attributes.hpp"
#include "Scene/ClassRegistry.hpp"
#include "Scene/Component.hpp"
#include "Scene/DrawList.hpp"
#include "Scene/PropValue.hpp"
#include "Scene/Scene.hpp"
#include "Scene/SceneComponent.hpp"
#include "Scene/Transform.hpp"

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using cinder::Scene::Actor;
using cinder::Scene::ClassRegistry;
using cinder::Scene::Component;
using cinder::Scene::DrawList;
using cinder::Scene::PropValue;
using cinder::Scene::Scene;
using cinder::Scene::SceneComponent;

namespace {

std::vector<std::string>& journal() {
    static std::vector<std::string> events;
    return events;
}

struct Counter : Actor {
    int starts = 0;
    int ticks = 0;
    int ends = 0;

    void beginPlay() override {
        starts++;
        journal().push_back("actor " + name());
    }
    void tick(float dt) override { ticks++; }
    void endPlay() override { ends++; }

    CINDER_ACTOR(Counter, Actor) {}
};

struct Probe : Component {
    int starts = 0;
    int ticks = 0;
    int renders = 0;
    int ends = 0;

    void beginPlay() override {
        starts++;
        journal().push_back("component " + name());
    }
    void tick(float dt) override { ticks++; }
    void onRender(float alpha, DrawList& draws) override { renders++; }
    void endPlay() override { ends++; }

    CINDER_COMPONENT(Probe, Component) {}
};

struct PlacedProbe : SceneComponent {
    int renders = 0;

    void onRender(float alpha, DrawList& draws) override { renders++; }

    CINDER_COMPONENT(PlacedProbe, SceneComponent) {}
};

struct NullDraws : DrawList {
    int textureHandle(std::string_view path) override { return WHITE; }
    int meshHandle(std::string_view name) override { return 0; }
    void background(float r, float g, float b) override {}
    void sprite(int texture, const glm::mat4& model, glm::vec2 size, const glm::vec4& color) override {}
    void mesh(int mesh, int texture, const glm::mat4& model) override {}
    void camera(const cinder::Scene::View& view) override {}
};

struct Suicide : Component {
    void tick(float dt) override { owner()->destroy(); }

    CINDER_COMPONENT(Suicide, Component) {}
};

struct Spawner : Component {
    Counter* spawned = nullptr;

    void tick(float dt) override {
        if (spawned == nullptr) spawned = scene()->spawn<Counter>(owner());
    }

    CINDER_COMPONENT(Spawner, Component) {}
};

struct Recorder {
    std::vector<std::string> events;

    void watch(Scene& scene, Actor& source, Actor& owner) {
        using cinder::Scene::Signal;
        using cinder::Scene::SignalArgs;
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
    ClassRegistry types;
    Scene scene{types};
};

Counter* named(Scene& scene, const char* name, Actor* parent = nullptr) {
    auto owned = std::make_unique<Counter>();
    owned->setName(name);
    return static_cast<Counter*>(scene.insert(std::move(owned), parent));
}

}

TEST_CASE("beginPlay runs once before the first tick") {
    Fixture f;
    Counter* counter = f.scene.spawn<Counter>();

    f.scene.update(0.1f);
    CHECK(counter->starts == 1);
    CHECK(counter->ticks == 1);

    f.scene.update(0.1f);
    CHECK(counter->starts == 1);
    CHECK(counter->ticks == 2);
}

TEST_CASE("components begin play before their actor, in order") {
    Fixture f;
    journal().clear();
    auto owned = std::make_unique<Counter>();
    owned->setName("Box");
    owned->addComponent<Probe>("first");
    owned->addComponent<Probe>("second");
    f.scene.insert(std::move(owned));

    f.scene.update(0.1f);
    CHECK(journal() == std::vector<std::string>{"component first", "component second", "actor Box"});
}

TEST_CASE("components render before they start and tick after") {
    Fixture f;
    Counter* actor = f.scene.spawn<Counter>();
    auto* probe = actor->addComponent<Probe>();
    NullDraws draws;

    f.scene.render(0.0f, draws);
    CHECK(probe->starts == 0);
    CHECK(probe->renders == 1);

    f.scene.update(0.1f);
    CHECK(probe->starts == 1);
    CHECK(probe->ticks == 1);
}

TEST_CASE("a component added to a running actor begins on the next frame") {
    Fixture f;
    Counter* actor = f.scene.spawn<Counter>();
    f.scene.update(0.1f);

    auto* late = actor->addComponent<Probe>();
    CHECK(late->starts == 0);
    f.scene.update(0.1f);
    CHECK(late->starts == 1);
    CHECK(late->ticks == 1);
}

TEST_CASE("actors spawned during a tick begin on the next frame") {
    Fixture f;
    Counter* host = f.scene.spawn<Counter>();
    auto* spawner = host->addComponent<Spawner>();

    f.scene.update(0.1f);
    REQUIRE(spawner->spawned != nullptr);
    CHECK(spawner->spawned->starts == 0);
    CHECK(spawner->spawned->ticks == 0);

    f.scene.update(0.1f);
    CHECK(spawner->spawned->starts == 1);
    CHECK(spawner->spawned->ticks == 1);
}

TEST_CASE("destroy is deferred to the end of the frame") {
    Fixture f;
    Counter* doomed = f.scene.spawn<Counter>();
    doomed->addComponent<Suicide>();
    const int id = doomed->id();
    Counter* sibling = f.scene.spawn<Counter>();

    f.scene.update(0.1f);
    CHECK(sibling->ticks == 1);
    CHECK(f.scene.byId(id) == nullptr);
}

TEST_CASE("destroying an actor destroys its components and attached actors") {
    Fixture f;
    Counter* parent = f.scene.spawn<Counter>();
    const int probe = parent->addComponent<Probe>()->id();
    Counter* child = f.scene.spawn<Counter>(parent);
    const int childId = child->id();

    f.scene.update(0.1f);
    CHECK(child->starts == 1);

    f.scene.destroy(parent);
    f.scene.update(0.1f);

    CHECK(f.scene.byId(childId) == nullptr);
    CHECK(f.scene.byId(probe) == nullptr);
    CHECK(f.scene.roots().empty());
}

TEST_CASE("destroying a component removes it and the components attached to it") {
    Fixture f;
    Counter* actor = f.scene.spawn<Counter>();
    auto* root = actor->addComponent<SceneComponent>("Root");
    auto* arm = actor->addComponent<SceneComponent>("Arm");
    auto* hand = actor->addComponent<Probe>("Hand");
    auto* finger = actor->addComponent<SceneComponent>("Finger");
    finger->attachTo(arm);
    const int armId = arm->id();
    const int fingerId = finger->id();

    f.scene.update(0.1f);
    f.scene.destroy(arm);
    CHECK(arm->destroyed());
    CHECK(finger->destroyed());
    f.scene.update(0.1f);

    CHECK(f.scene.byId(armId) == nullptr);
    CHECK(f.scene.byId(fingerId) == nullptr);
    CHECK(actor->components() == std::vector<Component*>{root, hand});

    f.scene.destroyNow(hand);
    CHECK(actor->components() == std::vector<Component*>{root});
}

TEST_CASE("destroyNow tears an actor down at once") {
    Fixture f;
    Counter* parent = f.scene.spawn<Counter>();
    const int childId = f.scene.spawn<Counter>(parent)->id();
    Counter* unstarted = f.scene.spawn<Counter>();
    const int unstartedId = unstarted->id();

    f.scene.destroyNow(unstarted);
    f.scene.update(0.1f);
    f.scene.destroyNow(parent);

    CHECK(f.scene.byId(childId) == nullptr);
    CHECK(f.scene.byId(unstartedId) == nullptr);
    CHECK(f.scene.roots().empty());
}

namespace {

struct Ending : Actor {
    std::vector<std::string>* log = nullptr;
    void endPlay() override { log->push_back("actor"); }
    CINDER_ACTOR(Ending, Actor) {}
};

struct EndingPart : Component {
    std::vector<std::string>* log = nullptr;
    void endPlay() override { log->push_back("component " + name()); }
    CINDER_COMPONENT(EndingPart, Component) {}
};

}

TEST_CASE("endPlay runs for started objects only, components last to first and the actor after") {
    Fixture f;
    std::vector<std::string> log;
    auto* actor = f.scene.spawn<Ending>();
    actor->log = &log;
    actor->addComponent<EndingPart>("a")->log = &log;
    actor->addComponent<EndingPart>("b")->log = &log;
    f.scene.update(0.1f);
    actor->addComponent<EndingPart>("unstarted")->log = &log;

    f.scene.destroyNow(actor);
    CHECK(log == std::vector<std::string>{"component b", "component a", "actor"});
}

TEST_CASE("a disabled actor begins play but neither ticks nor renders") {
    Fixture f;
    Counter* counter = f.scene.spawn<Counter>();
    auto* probe = counter->addComponent<Probe>();
    counter->setEnabled(false);
    NullDraws draws;

    f.scene.update(0.1f);
    f.scene.render(0.0f, draws);
    CHECK(counter->starts == 1);
    CHECK(counter->ticks == 0);
    CHECK(probe->starts == 1);
    CHECK(probe->ticks == 0);
    CHECK(probe->renders == 0);
}

TEST_CASE("a disabled actor skips the actors attached to it") {
    Fixture f;
    Counter* parent = f.scene.spawn<Counter>();
    Counter* child = f.scene.spawn<Counter>(parent);
    parent->setEnabled(false);

    f.scene.update(0.1f);
    CHECK(child->starts == 1);
    CHECK(child->ticks == 0);
    CHECK_FALSE(child->enabledInHierarchy());
}

TEST_CASE("a disabled component skips itself and the components attached to it") {
    Fixture f;
    Counter* actor = f.scene.spawn<Counter>();
    auto* root = actor->addComponent<PlacedProbe>("Root");
    auto* arm = actor->addComponent<PlacedProbe>("Arm");
    auto* hand = actor->addComponent<PlacedProbe>("Hand");
    auto* behaviour = actor->addComponent<Probe>("Behaviour");
    hand->attachTo(arm);
    arm->setEnabled(false);
    NullDraws draws;

    f.scene.render(0.0f, draws);
    CHECK(root->renders == 1);
    CHECK(arm->renders == 0);
    CHECK(hand->renders == 0);
    CHECK(behaviour->renders == 1);

    root->setEnabled(false);
    f.scene.render(0.0f, draws);
    CHECK(root->renders == 1);
    CHECK(behaviour->renders == 2);
}

TEST_CASE("the root is the first scene component that is not attached") {
    Fixture f;
    Counter* actor = f.scene.spawn<Counter>();
    CHECK(actor->rootComponent() == nullptr);
    CHECK(actor->transform() == nullptr);

    actor->addComponent<Probe>();
    auto* first = actor->addComponent<SceneComponent>("First");
    auto* second = actor->addComponent<SceneComponent>("Second");
    CHECK(actor->rootComponent() == first);
    CHECK(actor->transform() == &first->transform());

    actor->setRootComponent(second);
    CHECK(actor->rootComponent() == second);
    CHECK(actor->components().front() == second);

    second->attachTo(first);
    CHECK(actor->rootComponent() == first);
    CHECK(second->isAttachedTo(first));
}

TEST_CASE("ids are shared by actors and components and lookups resolve") {
    Fixture f;
    Counter* a = named(f.scene, "a");
    Counter* b = named(f.scene, "b", a);
    auto* probe = a->addComponent<Probe>("probe");

    CHECK(a->id() != b->id());
    CHECK(probe->id() != a->id());
    CHECK(probe->id() != b->id());
    CHECK(f.scene.byId(a->id()) == a);
    CHECK(f.scene.actor(a->id()) == a);
    CHECK(f.scene.actor(probe->id()) == nullptr);
    CHECK(f.scene.component(probe->id()) == probe);
    CHECK(f.scene.component(a->id()) == nullptr);
    CHECK(f.scene.find("b") == b);
    CHECK(a->findAttached("b") == b);
    CHECK(a->findComponent("probe") == probe);
    CHECK(a->findComponent<Probe>() == probe);
    CHECK(f.scene.find("nope") == nullptr);
}

TEST_CASE("an explicit id is honoured and raises the watermark") {
    Fixture f;
    Actor* loaded = f.scene.insert(std::make_unique<Counter>(), nullptr, 42);

    CHECK(loaded->id() == 42);
    CHECK(f.scene.byId(42) == loaded);
    CHECK(f.scene.spawn<Counter>()->id() == 43);
}

TEST_CASE("component ids can be given at insert") {
    Fixture f;
    auto owned = std::make_unique<Counter>();
    Component* probe = owned->addComponent<Probe>();
    Component* other = owned->addComponent<Probe>();
    Actor* actor = f.scene.insert(std::move(owned), nullptr, 5, {{probe, 9}});

    CHECK(actor->id() == 5);
    CHECK(probe->id() == 9);
    CHECK(other->id() == 10);
}

TEST_CASE("a taken id is rejected") {
    Fixture f;
    const int id = f.scene.spawn<Counter>()->id();
    CHECK_THROWS_AS(f.scene.insert(std::make_unique<Counter>(), nullptr, id), std::runtime_error);
}

TEST_CASE("an explicit id below the watermark does not lower it") {
    Fixture f;
    f.scene.insert(std::make_unique<Counter>(), nullptr, 10);
    f.scene.insert(std::make_unique<Counter>(), nullptr, 3);

    CHECK(f.scene.spawn<Counter>()->id() == 11);
}

TEST_CASE("clear does not reissue ids") {
    Fixture f;
    const int before = f.scene.spawn<Counter>()->id();

    f.scene.clear();
    Actor* after = f.scene.spawn<Counter>();

    CHECK(before != after->id());
    CHECK(f.scene.byId(before) == nullptr);
}

TEST_CASE("unregistered classes are named generically, registered ones after their class") {
    Fixture f;
    cinder::Components::registerBuiltins(f.types);

    Actor* counter = f.scene.spawn<Counter>();
    CHECK(counter->name() == "Actor");
    CHECK(counter->addComponent<Probe>()->name() == "Component");
    CHECK(f.scene.spawn("StaticMeshActor")->name() == "StaticMeshActor");
    CHECK(f.scene.addComponent(*counter, "SpriteComponent")->name() == "SpriteComponent");
    CHECK(f.scene.spawn("Nope") == nullptr);
    CHECK(f.scene.spawn("SpriteComponent") == nullptr);
    CHECK(f.scene.addComponent(*counter, "Actor") == nullptr);
}

TEST_CASE("components made by the actor's constructor are defaults") {
    Fixture f;
    cinder::Components::registerBuiltins(f.types);
    auto* actor = f.scene.spawn<cinder::Components::StaticMeshActor>();
    Component* added = f.scene.addComponent(*actor, "SpriteComponent");

    CHECK(actor->mesh().isDefault());
    CHECK(actor->rootComponent() == &actor->mesh());
    CHECK_FALSE(added->isDefault());
    CHECK(f.scene.archetype(actor->mesh()) != nullptr);
    CHECK(f.scene.archetype(actor->mesh()) != &actor->mesh());
}

TEST_CASE("clone copies props, components, transform, attributes and attached actors with fresh ids") {
    Fixture f;
    cinder::Components::registerBuiltins(f.types);
    auto* box = f.scene.spawn<cinder::Components::StaticMeshActor>();
    box->setName("Box");
    box->mesh().setMesh("sphere");
    box->transform()->setPosition(1, 2, 3);
    box->setAttribute("hp", PropValue::integer(3));
    auto* sprite = box->addComponent<cinder::Components::SpriteComponent>("Badge");
    sprite->setSize(5, 7);
    auto* tip = box->addComponent<SceneComponent>("Tip");
    tip->attachTo(sprite);
    f.scene.spawn("Actor", box)->setName("Rider");

    Actor* copy = f.scene.clone(*box, nullptr);
    REQUIRE(copy != nullptr);
    CHECK(copy->id() != box->id());
    CHECK(copy->name() == "Box");
    CHECK(copy->transform()->position().y == doctest::Approx(2));
    CHECK(copy->attribute("hp")->as<std::int64_t>() == 3);
    REQUIRE(copy->components().size() == 3);

    auto* mesh = dynamic_cast<cinder::Components::StaticMeshComponent*>(copy->rootComponent());
    REQUIRE(mesh != nullptr);
    CHECK(mesh->mesh() == "sphere");
    CHECK(mesh->isDefault());
    auto* badge = dynamic_cast<cinder::Components::SpriteComponent*>(copy->findComponent("Badge"));
    REQUIRE(badge != nullptr);
    CHECK(badge != sprite);
    CHECK(badge->size().y == doctest::Approx(7));
    CHECK_FALSE(badge->isDefault());
    auto* copiedTip = dynamic_cast<SceneComponent*>(copy->findComponent("Tip"));
    REQUIRE(copiedTip != nullptr);
    CHECK(copiedTip->attachParent() == badge);
    REQUIRE(copy->attachedActors().size() == 1);
    CHECK(copy->attachedActors()[0]->name() == "Rider");
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
    child->attachTo(nullptr);
    child->attachTo(parent);
    f.scene.destroy(child);
    f.scene.update(0.1f);

    const std::vector<std::string> expected{"added child", "attribute hp", "removed child",
                                            "added child", "removed child", "destroying child"};
    CHECK(recorder.events == expected);
}

TEST_CASE("an attribute notifies once per real change") {
    Fixture f;
    Recorder recorder;
    Actor* a = f.scene.spawn<Counter>();
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
    Actor* a = f.scene.spawn<Counter>();
    f.scene.update(0.0f);
    recorder.watch(f.scene, *a, *a);

    a->loadAttributes({{"rate", PropValue::number(2.0)}});

    CHECK(recorder.events.empty());
    CHECK(a->attribute("rate")->as<double>() == doctest::Approx(2.0));
}

TEST_CASE("attributes hold numbers, strings, bools and small vectors") {
    using cinder::Scene::isAttributeName;
    using cinder::Scene::isAttributeValue;

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

struct Listener : Actor {
    std::vector<std::string> attributes;
    std::vector<int> added;
    std::vector<int> removed;
    std::vector<int> components;

    void onAttributeChanged(std::string_view name) override { attributes.emplace_back(name); }
    void onChildAdded(Actor& child) override { added.push_back(child.id()); }
    void onChildRemoved(Actor& child) override { removed.push_back(child.id()); }
    void onComponentAdded(Component& component) override { components.push_back(component.id()); }

    CINDER_ACTOR(Listener, Actor) {}
};

}

TEST_CASE("event hooks reach only started actors, so nothing runs in Edit mode") {
    ClassRegistry types;
    Scene scene(types);
    auto* listener = scene.spawn<Listener>();

    listener->setAttribute("hp", PropValue::integer(3));
    scene.spawn<Counter>(listener);
    listener->addComponent<Probe>();
    CHECK(listener->attributes.empty());
    CHECK(listener->added.empty());
    CHECK(listener->components.empty());

    scene.update(0.0f);
    listener->setAttribute("hp", PropValue::integer(4));
    const int child = scene.spawn<Counter>(listener)->id();
    scene.destroyNow(scene.actor(child));
    const int probe = listener->addComponent<Probe>()->id();

    CHECK(listener->attributes == std::vector<std::string>{"hp"});
    CHECK(listener->added == std::vector<int>{child});
    CHECK(listener->removed == std::vector<int>{child});
    CHECK(listener->components == std::vector<int>{probe});
}

TEST_CASE("a connection hears another object's signal and dies with either") {
    ClassRegistry types;
    Scene scene(types);
    auto* source = scene.spawn<Counter>();
    auto* owner = source->addComponent<Probe>();
    std::vector<std::string> heard;
    scene.connect(*source, cinder::Scene::Signal::AttributeChanged, *owner,
                  [&heard](const cinder::Scene::SignalArgs& args) { heard.emplace_back(args.name); });

    source->setAttribute("before", PropValue::integer(1));
    scene.update(0.0f);
    source->setAttribute("speed", PropValue::integer(2));
    CHECK(heard == std::vector<std::string>{"speed"});

    scene.destroyNow(owner);
    source->setAttribute("after", PropValue::integer(3));
    CHECK(heard.size() == 1);

    auto* other = scene.spawn<Counter>();
    scene.update(0.0f);
    int destroying = 0;
    scene.connect(*source, cinder::Scene::Signal::Destroying, *other,
                  [&destroying](const cinder::Scene::SignalArgs&) { destroying++; });
    const int handle = scene.connect(*source, cinder::Scene::Signal::AttributeChanged, *other,
                                     [&heard](const cinder::Scene::SignalArgs&) { heard.emplace_back("x"); });
    scene.disconnect(handle);
    source->setAttribute("quiet", PropValue::integer(4));
    CHECK(heard.size() == 1);
    scene.destroyNow(source);
    CHECK(destroying == 1);
}

TEST_CASE("timers fire on scene time, repeat, and are owned by an object") {
    ClassRegistry types;
    Scene scene(types);
    auto* owner = scene.spawn<Counter>();
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

    auto* probe = owner->addComponent<Probe>();
    int orphaned = 0;
    scene.after(*probe, 0.1, [&orphaned] { orphaned++; });
    scene.destroy(probe);
    scene.after(*owner, 0.1, [&orphaned] { orphaned++; });
    scene.destroy(owner);
    for (int i = 0; i < 4; ++i) scene.update(0.125f);
    CHECK(orphaned == 0);

    auto* survivor = scene.spawn<Counter>();
    scene.after(*survivor, 1.0, [&orphaned] { orphaned++; });
    scene.clear();
    CHECK(scene.time() == 0.0);
}

namespace {

struct First final : Actor {
    CINDER_ACTOR(First, Actor) {}
};

struct Second final : Actor {
    CINDER_ACTOR(Second, Actor) {}
};

}

TEST_CASE("a removed class keeps its slot and takes a different type when added again") {
    ClassRegistry types;
    types.add<Counter>("Before");
    types.add<First>("Swapped");
    types.add<Probe>("After");
    types.remove("Swapped");
    CHECK(types.createActor("Swapped") == nullptr);
    CHECK(types.fallback("Swapped") == nullptr);
    CHECK(types.nameOf(First{}).empty());

    types.add<Second>("Swapped");
    CHECK(types.nameOf(*types.createActor("Swapped")) == "Swapped");
    CHECK(types.nameOf(Second{}) == "Swapped");
    REQUIRE(types.registered().size() == 3);
    CHECK(types.registered()[1].name == "Swapped");
    CHECK(types.registered()[1].kind == cinder::Scene::ClassKind::Actor);
    CHECK(types.registered()[2].kind == cinder::Scene::ClassKind::Component);
    CHECK(types.createComponent("Before") == nullptr);
    CHECK(types.createActor("After") == nullptr);
}
