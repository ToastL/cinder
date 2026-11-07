#include <doctest/doctest.h>

#include "Core/GameModule.hpp"
#include "Nodes/Builtins.hpp"
#include "Scene/Node.hpp"
#include "Scene/NodeTypes.hpp"
#include "Scene/PropValue.hpp"
#include "Scene/Scene.hpp"
#include "Scene/Transform.hpp"
#include "Serial/SceneCodec.hpp"

#include <string>
#include <vector>

using cinder::Core::GameModule;
using cinder::Scene::NodeTypes;
using cinder::Scene::Scene;
using cinder::Serial::SceneCodec;

namespace {

const std::string SCENE = R"(version 4
nodes {
    TestTicker {
        id 1
        step 2
    }
    TestMover {
        id 2
        position 1 0 0
    }
}
)";

}

TEST_CASE("a game module registers its classes, runs them, and unloads cleanly") {
    NodeTypes types;
    cinder::Nodes::registerBuiltins(types);
    Scene scene(types);

    std::string error;
    std::unique_ptr<GameModule> module = GameModule::open(CINDER_TEST_MODULE, types, error);
    REQUIRE_MESSAGE(module != nullptr, error);
    CHECK(types.create("TestTicker") == nullptr);

    module->enlist();
    CHECK(module->classes() == std::vector<std::string>{"TestTicker", "TestMover"});
    CHECK(types.nameOf(*types.create("Group")) == "Group");

    SceneCodec::load(SCENE, scene);
    scene.update(0.5f);
    scene.update(0.5f);
    REQUIRE(scene.byId(1)->findFirstChild("Made") != nullptr);
    CHECK(types.nameOf(*scene.byId(1)->findFirstChild("Made")) == "Group");
    scene.destroyNow(scene.byId(1)->findFirstChild("Made"));
    const cinder::Scene::PropValue* count = scene.byId(1)->attribute("count");
    REQUIRE(count != nullptr);
    CHECK(count->as<double>() == doctest::Approx(4.0));
    CHECK(scene.byId(2)->transform()->position().x == doctest::Approx(3.0f));

    scene.byId(1)->removeAttribute("count");
    const std::string text = SceneCodec::save(scene);
    scene.clear();
    module.reset();
    CHECK(types.create("TestTicker") == nullptr);
    CHECK(types.fallback("TestMover") == nullptr);

    SceneCodec::load(text, scene);
    CHECK(types.nameOf(*scene.byId(1)) == "TestTicker");
    CHECK(SceneCodec::save(scene) == text);

    module = GameModule::open(CINDER_TEST_MODULE, types, error);
    REQUIRE_MESSAGE(module != nullptr, error);
    module->enlist();
    SceneCodec::load(text, scene);
    scene.update(1.0f);
    CHECK(scene.byId(1)->attribute("count")->as<double>() == doctest::Approx(2.0));
    scene.clear();
}

TEST_CASE("a module built for another engine is refused") {
    NodeTypes types;
    std::string error;
    CHECK(GameModule::open(CINDER_STALE_MODULE, types, error) == nullptr);
    CHECK(error.find("0.0.0-stale") != std::string::npos);
    CHECK(GameModule::open("/nowhere/missing.dylib", types, error) == nullptr);
}
