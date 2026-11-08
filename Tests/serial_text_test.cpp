#include <doctest/doctest.h>

#include "Components/Actors.hpp"
#include "Components/Builtins.hpp"
#include "Scene/Actor.hpp"
#include "Scene/ClassRegistry.hpp"
#include "Scene/SceneComponent.hpp"
#include "Scene/Scene.hpp"
#include "Serial/SceneCodec.hpp"

#include <clocale>
#include <limits>
#include <memory>
#include <string>

using cinder::Components::CameraActor;
using cinder::Components::CameraComponent;
using cinder::Components::SpriteActor;
using cinder::Scene::Actor;
using cinder::Scene::ClassRegistry;
using cinder::Scene::SceneComponent;
using cinder::Scene::Scene;
using cinder::Serial::SceneCodec;

namespace {

struct Tally : cinder::Scene::Component {
    int count_ = 0;

    CINDER_COMPONENT(Tally, cinder::Scene::Component) { CINDER_PROP(count_); }
};

struct Fixture {
    ClassRegistry types;
    Scene scene{types};

    Fixture() {
        cinder::Components::registerBuiltins(types);
        types.add<Tally>("Tally");
    }

    Actor* group(Actor* parent = nullptr) {
        Actor* actor = scene.spawn<Actor>(parent);
        actor->addComponent<SceneComponent>();
        return actor;
    }
};

bool contains(const std::string& text, std::string_view needle) {
    return text.find(needle) != std::string::npos;
}

}

TEST_CASE("integral floats lose the decimal point") {
    Fixture f;
    f.group()->transform()->setPosition(0, 3, 8);

    CHECK(contains(SceneCodec::save(f.scene), "position 0 3 8"));
}

TEST_CASE("fractional floats use the shortest round trip") {
    Fixture f;
    f.group()->transform()->setPosition(0.1f, 0, 0);

    CHECK(contains(SceneCodec::save(f.scene), "position 0.1 0 0"));
}

TEST_CASE("int props emit as integers") {
    Fixture f;
    f.group()->addComponent<Tally>()->count_ = 3;

    CHECK(contains(SceneCodec::save(f.scene), "count 3"));
}

TEST_CASE("asset paths emit quoted") {
    Fixture f;
    f.scene.spawn<SpriteActor>()->sprite().setTexture("tiles/grass.png");

    CHECK(contains(SceneCodec::save(f.scene), "texture \"tiles/grass.png\""));
}

TEST_CASE("enums emit lowercase") {
    Fixture f;
    f.scene.spawn<CameraActor>()->camera().setProjection(CameraComponent::Projection::Orthographic);

    CHECK(contains(SceneCodec::save(f.scene), "projection \"orthographic\""));
}

TEST_CASE("strings are quoted and escaped") {
    Fixture f;
    f.group()->setName("a\"b\\c\td");

    const std::string text = SceneCodec::save(f.scene);
    CHECK(contains(text, "name \"a\\\"b\\\\c\\td\""));

    SceneCodec::load(text, f.scene);
    CHECK(f.scene.roots().front()->name() == "a\"b\\c\td");
}

TEST_CASE("non-finite floats become zero") {
    Fixture f;
    Actor* a = f.group();
    a->transform()->position() = glm::vec3(std::numeric_limits<float>::quiet_NaN(),
                                           std::numeric_limits<float>::infinity(), 1.0f);
    a->transform()->dirty();

    CHECK(contains(SceneCodec::save(f.scene), "position 0 0 1"));
}

TEST_CASE("empty collections are omitted") {
    Fixture f;
    CHECK(SceneCodec::save(f.scene) == "version 6\n");
}

TEST_CASE("output is independent of the default locale") {
    Fixture f;
    CameraComponent& camera = f.scene.spawn<CameraActor>()->camera();
    camera.transform().setPosition(0.5f, 0, 0);
    camera.setProjection(CameraComponent::Projection::Orthographic);

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
    Actor* root = f.scene.insert(std::make_unique<Actor>(), nullptr, 1);
    Actor* mid = f.scene.insert(std::make_unique<Actor>(), root, 2);
    auto deep = std::make_unique<Actor>();
    auto* base = deep->addComponent<SceneComponent>("Base");
    deep->addComponent<SceneComponent>("Tip")->attachTo(base);
    f.scene.insert(std::move(deep), mid, 3);

    const std::string first = SceneCodec::save(f.scene);
    CHECK(contains(first, "children {"));
    CHECK(contains(first, "attached {"));
    SceneCodec::load(first, f.scene);

    CHECK(SceneCodec::save(f.scene) == first);
    CHECK(f.scene.actor(3)->attachParent() == f.scene.actor(2));
    auto* tip = dynamic_cast<SceneComponent*>(f.scene.actor(3)->findComponent("Tip"));
    REQUIRE(tip != nullptr);
    CHECK(tip->attachParent() == f.scene.actor(3)->findComponent("Base"));
    CHECK(f.scene.roots().size() == 1);
}
