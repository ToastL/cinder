#include <doctest/doctest.h>

#include "test_support.hpp"

#include "Core/GameModule.hpp"
#include "Dev/GameBuild.hpp"
#include "Dev/ModuleHost.hpp"
#include "Components/Builtins.hpp"
#include "Platform/Files.hpp"
#include "Scene/Actor.hpp"
#include "Scene/ClassRegistry.hpp"
#include "Scene/Scene.hpp"
#include "Scene/Transform.hpp"
#include "Serial/SceneCodec.hpp"

#include <chrono>
#include <filesystem>
#include <string>
#include <thread>

using cinder::Dev::ModuleHost;
using cinder::Dev::GameBuild;
using cinder::Scene::ClassRegistry;
using cinder::Scene::Scene;

namespace {

std::string hopper(const std::string& body) {
    return R"(#include "Game/Game.hpp"

class Hopper final : public cinder::Scene::SceneComponent {
public:
    CINDER_COMPONENT(Hopper, cinder::Scene::SceneComponent) { CINDER_PROP(step_); }
    void tick(float dt) override {
        glm::vec3 p = transform().position();
        )" + body + R"(
        transform().setPosition(p.x, p.y, p.z);
    }
private:
    float step_ = 1.0f;
};

CINDER_REGISTER(Hopper);
)";
}

void write(const std::filesystem::path& path, const std::string& text) {
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    cinder::Platform::writeTextFile(path, text);
}

void finish(ModuleHost& host) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::minutes(2);
    while (host.building() && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    host.update();
}

}

TEST_CASE("hot reload: a project's C++ builds, loads, swaps and survives a broken edit") {
    const GameBuild build{CINDER_CBT, CINDER_OUTPUT_ROOT, CINDER_CONFIG, std::string(GameBuild::defaultBackend())};

    const std::filesystem::path project = testing::uniqueTempDir("cinder_hot_reload");
    std::filesystem::create_directories(project / "Source");
    write(project / "Game.cinder", "{\"fileVersion\": 1, \"engineAssociation\": \"\"}\n");
    const std::filesystem::path source = project / "Source" / "Hopper.cpp";
    write(source, hopper("p.x += step_;"));

    ClassRegistry types;
    cinder::Components::registerBuiltins(types);
    Scene scene(types);
    {
        ModuleHost host(scene, project, "Game", build);
        REQUIRE(host.buildNow());
        REQUIRE(host.module() != nullptr);
        CHECK(host.lastResult().compiled == 2);
        CHECK(std::filesystem::exists(project / "compile_commands.json"));

        cinder::Serial::SceneCodec::load("version 5\nactors {\n    Actor {\n        id 1\n        components {\n"
                                         "            Hopper {\n                id 2\n                step 3\n"
                                         "            }\n        }\n    }\n}\n", scene);
        scene.update(0.1f);
        CHECK(scene.actor(1)->transform()->position().x == doctest::Approx(3.0f));

        REQUIRE(host.buildNow());
        CHECK(host.lastResult().compiled == 0);
        CHECK_FALSE(host.lastResult().linked);

        write(source, hopper("p.y += step_;"));
        REQUIRE(host.requestBuild());
        finish(host);
        REQUIRE(host.reloadPending());
        REQUIRE(host.reload());
        CHECK(host.lastResult().compiled == 1);
        scene.update(0.1f);
        CHECK(scene.actor(1)->transform()->position().x == doctest::Approx(3.0f));
        CHECK(scene.actor(1)->transform()->position().y == doctest::Approx(3.0f));

        write(source, hopper("p.z += step_ +;"));
        REQUIRE(host.requestBuild());
        finish(host);
        CHECK_FALSE(host.lastResult().ok);
        CHECK(host.lastResult().errors > 0);
        CHECK_FALSE(host.reloadPending());
        scene.update(0.1f);
        CHECK(scene.actor(1)->transform()->position().y == doctest::Approx(6.0f));
    }
    CHECK(scene.roots().empty());
    CHECK(types.createComponent("Hopper") == nullptr);
    std::filesystem::remove_all(project);
}
