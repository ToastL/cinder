#include <doctest/doctest.h>

#include "Components/Actors.hpp"
#include "Components/Builtins.hpp"
#include "Components/SpriteComponent.hpp"
#include "Reflect/Reflect.hpp"
#include "Scene/Actor.hpp"
#include "Scene/ClassRegistry.hpp"
#include "Scene/Component.hpp"
#include "Scene/Ref.hpp"
#include "Scene/Scene.hpp"
#include "Scene/SceneComponent.hpp"
#include "Serial/SceneCodec.hpp"

#include <memory>
#include <string>

using cinder::Components::SpriteComponent;
using cinder::Components::StaticMeshActor;
using cinder::Scene::Actor;
using cinder::Scene::ClassRegistry;
using cinder::Scene::Component;
using cinder::Scene::Ref;
using cinder::Scene::Scene;
using cinder::Scene::SceneComponent;
using cinder::Serial::SceneCodec;

namespace {

struct Pointer final : Component {
    Ref<Actor> target_;
    Ref<SpriteComponent> badge_;

    CINDER_COMPONENT(Pointer, Component) {
        CINDER_PROP(target_);
        CINDER_PROP(badge_);
    }
};

struct Fixture {
    ClassRegistry types;
    Scene scene{types};

    Fixture() {
        cinder::Components::registerBuiltins(types);
        types.add<Pointer>("Pointer");
    }

    Actor* actor(const char* name, Actor* parent = nullptr) {
        Actor* made = scene.spawn("Actor", parent);
        made->setName(name);
        return made;
    }

    Pointer* pointer(Actor& on) { return static_cast<Pointer*>(scene.addComponent(on, "Pointer")); }

    void reload() { SceneCodec::load(SceneCodec::save(scene), scene); }

    Pointer* findPointer(const char* actorName) {
        Actor* found = scene.find(actorName);
        return found == nullptr ? nullptr : found->findComponent<Pointer>();
    }
};

const cinder::Reflect::PropDef& prop(std::string_view name) {
    for (const cinder::Reflect::PropDef& def : cinder::Reflect::props<Pointer>()) {
        if (def.name() == name) return def;
    }
    FAIL("no such prop: " << name);
    return cinder::Reflect::props<Pointer>().front();
}

bool contains(const std::string& text, std::string_view needle) {
    return text.find(needle) != std::string::npos;
}

}

TEST_CASE("a Ref prop reads and writes its id and accepts only its class") {
    Fixture f;
    Actor* a = f.actor("A");
    Pointer* p = f.pointer(*a);
    auto* sprite = a->addComponent<SpriteComponent>("Badge");

    const cinder::Reflect::PropDef& target = prop("target");
    const cinder::Reflect::PropDef& badge = prop("badge");
    CHECK(target.type() == cinder::Reflect::PropType::Ref);
    CHECK(target.arity() == 0);

    target.writeInt(p, a->id());
    CHECK(p->target_.id == a->id());
    CHECK(target.readInt(p) == a->id());

    const cinder::Scene::Object* actorObject = a;
    const cinder::Scene::Object* spriteObject = sprite;
    const cinder::Scene::Object* pointerObject = p;
    CHECK(target.accepts(actorObject));
    CHECK_FALSE(target.accepts(spriteObject));
    CHECK(badge.accepts(spriteObject));
    CHECK_FALSE(badge.accepts(pointerObject));
    CHECK_FALSE(badge.accepts(nullptr));

    Pointer copy;
    cinder::Reflect::copyProps(cinder::Reflect::props<Pointer>(), p, &copy);
    CHECK(copy.target_.id == a->id());
}

TEST_CASE("get resolves live objects of the right class and nothing else") {
    Fixture f;
    Actor* a = f.actor("A");
    Actor* b = f.actor("B");
    Pointer* p = f.pointer(*a);

    CHECK(p->target_.get(f.scene) == nullptr);
    p->target_.set(b);
    CHECK(p->target_.get(f.scene) == b);

    p->badge_.id = b->id();
    CHECK(p->badge_.get(f.scene) == nullptr);

    p->target_.id = 9999;
    CHECK(p->target_.get(f.scene) == nullptr);

    p->target_.set(b);
    f.scene.destroy(b);
    CHECK(p->target_.get(f.scene) == nullptr);
    f.scene.update(0.0f);
    CHECK(p->target_.get(f.scene) == nullptr);

    p->target_.reset();
    CHECK_FALSE(p->target_);
}

TEST_CASE("clone remaps references inside the copied subtree and keeps the rest") {
    Fixture f;
    Actor* outside = f.actor("Outside");
    Actor* turret = f.actor("Turret");
    Actor* barrel = f.actor("Barrel", turret);
    auto* sprite = turret->addComponent<SpriteComponent>("Badge");
    Pointer* aim = f.pointer(*turret);
    aim->target_.set(barrel);
    aim->badge_.set(sprite);
    Pointer* back = f.pointer(*barrel);
    back->target_.set(outside);

    Actor* copy = f.scene.clone(*turret, nullptr);
    REQUIRE(copy != nullptr);
    REQUIRE(copy->attachedActors().size() == 1);
    Actor* copiedBarrel = copy->attachedActors()[0];
    auto* copiedAim = copy->findComponent<Pointer>();
    auto* copiedBack = copiedBarrel->findComponent<Pointer>();
    REQUIRE(copiedAim != nullptr);
    REQUIRE(copiedBack != nullptr);

    CHECK(copiedAim->target_.get(f.scene) == copiedBarrel);
    CHECK(copiedAim->badge_.get(f.scene) == copy->findComponent("Badge"));
    CHECK(copiedAim->badge_.get(f.scene) != sprite);
    CHECK(copiedBack->target_.get(f.scene) == outside);
    CHECK(aim->target_.get(f.scene) == barrel);
}

TEST_CASE("references round trip through a save, forward references included") {
    Fixture f;
    Actor* a = f.actor("A");
    Actor* b = f.actor("B");
    auto* sprite = b->addComponent<SpriteComponent>("Badge");
    Pointer* p = f.pointer(*a);
    p->target_.set(b);
    p->badge_.set(sprite);

    const std::string text = SceneCodec::save(f.scene);
    CHECK(contains(text, "target " + std::to_string(b->id())));
    f.reload();
    CHECK(SceneCodec::save(f.scene) == text);

    Pointer* loaded = f.findPointer("A");
    REQUIRE(loaded != nullptr);
    CHECK(loaded->target_.get(f.scene) == f.scene.find("B"));
    CHECK(loaded->badge_.get(f.scene) == f.scene.find("B")->findComponent("Badge"));
}

TEST_CASE("a reference to a deleted or mismatched object is not saved") {
    Fixture f;
    Actor* a = f.actor("A");
    Actor* b = f.actor("B");
    Pointer* p = f.pointer(*a);
    p->target_.set(b);
    p->badge_.id = b->id();
    f.scene.destroyNow(b);

    const std::string text = SceneCodec::save(f.scene);
    CHECK_FALSE(contains(text, "target"));
    CHECK_FALSE(contains(text, "badge"));
}

namespace {

std::string holderPointingAt(const std::string& target, const std::string& rest) {
    return "version 5\n"
           "actors {\n"
           "    Actor {\n"
           "        id 2\n"
           "        name \"Holder\"\n"
           "        components {\n"
           "            Pointer {\n"
           "                id 3\n"
           "                target " + target + "\n"
           "            }\n"
           "        }\n"
           "    }\n" + rest + "}\n";
}

std::string actorBlock(const std::string& id, const std::string& name) {
    return "    Actor {\n        id " + id + "\n        name \"" + name + "\"\n    }\n";
}

}

TEST_CASE("a reference follows an object whose file id was out of range to its fresh id") {
    Fixture f;
    SceneCodec::load(holderPointingAt("-5", actorBlock("-5", "Target")), f.scene);
    Pointer* holder = f.findPointer("Holder");
    REQUIRE(holder != nullptr);
    Actor* target = f.scene.find("Target");
    REQUIRE(target != nullptr);
    CHECK(target->id() > 0);
    CHECK(holder->target_.get(f.scene) == target);
}

TEST_CASE("a reference to a duplicated id goes to the object that kept it") {
    Fixture f;
    SceneCodec::load(holderPointingAt("7", actorBlock("7", "First") + actorBlock("7", "Second")), f.scene);
    Pointer* holder = f.findPointer("Holder");
    REQUIRE(holder != nullptr);
    CHECK(holder->target_.get(f.scene) == f.scene.find("First"));
    CHECK(f.scene.find("Second")->id() != 7);
}

TEST_CASE("a reference to an id that is not in the file loads empty") {
    Fixture f;
    f.actor("Filler");
    SceneCodec::load(holderPointingAt("40", actorBlock("7", "Other")), f.scene);
    Pointer* holder = f.findPointer("Holder");
    REQUIRE(holder != nullptr);
    CHECK(holder->target_.id == 0);

    SceneCodec::load(holderPointingAt("3", actorBlock("7", "Other")), f.scene);
    holder = f.findPointer("Holder");
    REQUIRE(holder != nullptr);
    CHECK(holder->target_.id == 0);
}
