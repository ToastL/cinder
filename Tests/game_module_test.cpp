#include <doctest/doctest.h>

#include "Core/GameModule.hpp"
#include "Components/Builtins.hpp"
#include "Scene/Actor.hpp"
#include "Scene/ClassRegistry.hpp"
#include "Scene/PropValue.hpp"
#include "Scene/Scene.hpp"
#include "Scene/Transform.hpp"
#include "Serial/SceneCodec.hpp"

#include <string>
#include <vector>

using cinder::Core::GameModule;
using cinder::Scene::ClassRegistry;
using cinder::Scene::Scene;
using cinder::Serial::SceneCodec;

namespace {

const std::string SCENE = R"(version 5
actors {
    Actor {
        id 1
        components {
            TestTicker {
                id 3
                step 2
            }
        }
    }
    Actor {
        id 2
        components {
            TestMover {
                id 4
                position 1 0 0
            }
        }
    }
}
)";

}

TEST_CASE("a game module registers its classes, runs them, and unloads cleanly") {
    ClassRegistry types;
    cinder::Components::registerBuiltins(types);
    Scene scene(types);

    std::string error;
    std::unique_ptr<GameModule> module = GameModule::open(CINDER_TEST_MODULE, types, error);
    REQUIRE_MESSAGE(module != nullptr, error);
    CHECK(types.createComponent("TestTicker") == nullptr);

    module->enlist();
    CHECK(module->classes() == std::vector<std::string>{"TestTicker", "TestMover"});
    CHECK(types.nameOf(*types.createActor("Actor")) == "Actor");
    CHECK(types.createComponent("Actor") == nullptr);

    SceneCodec::load(SCENE, scene);
    scene.update(0.5f);
    scene.update(0.5f);
    REQUIRE(scene.actor(1)->findAttached("Made") != nullptr);
    CHECK(types.nameOf(*scene.actor(1)->findAttached("Made")) == "Actor");
    scene.destroyNow(scene.actor(1)->findAttached("Made"));
    const cinder::Scene::PropValue* count = scene.actor(1)->attribute("count");
    REQUIRE(count != nullptr);
    CHECK(count->as<double>() == doctest::Approx(4.0));
    CHECK(scene.actor(2)->transform()->position().x == doctest::Approx(3.0f));

    scene.actor(1)->removeAttribute("count");
    const std::string text = SceneCodec::save(scene);
    scene.clear();
    module.reset();
    CHECK(types.createComponent("TestTicker") == nullptr);
    CHECK(types.fallback("TestMover") == nullptr);

    SceneCodec::load(text, scene);
    CHECK(types.nameOf(*scene.byId(3)) == "TestTicker");
    CHECK(scene.actor(2)->rootComponent() == nullptr);
    CHECK(SceneCodec::save(scene) == text);

    module = GameModule::open(CINDER_TEST_MODULE, types, error);
    REQUIRE_MESSAGE(module != nullptr, error);
    module->enlist();
    SceneCodec::load(text, scene);
    scene.update(1.0f);
    CHECK(scene.actor(1)->attribute("count")->as<double>() == doctest::Approx(2.0));
    scene.clear();
}

TEST_CASE("a module built for another engine is refused") {
    ClassRegistry types;
    std::string error;
    CHECK(GameModule::open(CINDER_STALE_MODULE, types, error) == nullptr);
    CHECK(error.find("0.0.0-stale") != std::string::npos);
    CHECK(GameModule::open("/nowhere/missing.dylib", types, error) == nullptr);
}
