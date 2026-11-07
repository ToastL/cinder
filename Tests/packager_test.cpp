#include <doctest/doctest.h>

#include "test_support.hpp"

#include "core/GameModule.hpp"
#include "dev/Packager.hpp"
#include "dev/Staging.hpp"
#include "dev/GameBuild.hpp"
#include "platform/Files.hpp"
#include "scene/NodeTypes.hpp"

#include <chrono>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <thread>

using cinder::dev::Configuration;
using cinder::dev::PackageTools;
using cinder::dev::Packager;
using cinder::dev::Platform;
using cinder::dev::StageRequest;

namespace {

PackageTools tools() { return PackageTools{"/b/packager"}; }

const std::string HOST(cinder::dev::platformName(cinder::dev::hostPlatform()));

struct Project {
    std::filesystem::path root;
    std::filesystem::path engine;
    std::filesystem::path shaders;
    std::filesystem::path player;

    explicit Project(const std::string& name, const std::string& descriptor) {
        const std::filesystem::path base = testing::uniqueTempDir("cinder_stage_" + name);
        root = base / "Game";
        engine = base / "engine";
        shaders = base / "shaders";
        player = base / "player";
        write(root / "Game.cinder", descriptor);
        write(root / "Config" / "Game.ini", "[Game]\ntitle=Game\n");
        write(root / "Content" / "Scenes" / "main.scene", "version 4\n");
        write(root / "Source" / "notes.txt", "not code\n");
        write(root / "Saved" / "junk.txt", "never shipped\n");
        write(engine / "Content" / "Fonts" / "Roboto.ttf", "font");
        write(shaders / "sprite.spv", "spv");
        write(shaders / "sprite.metallib", "metallib");
        write(shaders / "sprite.dxil", "dxil");
        write(player, "#!/bin/sh\n");
        std::filesystem::permissions(player, std::filesystem::perms::owner_all);
    }

    ~Project() { std::filesystem::remove_all(root.parent_path()); }

    static void write(const std::filesystem::path& path, const std::string& text) {
        std::filesystem::create_directories(path.parent_path());
        cinder::platform::writeTextFile(path, text);
    }

    StageRequest request(Configuration configuration = Configuration::Development) const {
        StageRequest out;
        out.project = root;
        out.platform = cinder::dev::hostPlatform();
        out.configuration = configuration;
        out.out = root.parent_path() / "out";
        out.player = player;
        out.engineDir = engine;
        out.shaderDir = shaders;
        return out;
    }
};

}

TEST_CASE("every platform is listed, and only the host desktop can package today") {
    CHECK(cinder::dev::platforms().size() == 5);
    CHECK(cinder::dev::platformName(Platform::IOS) == "iOS");
    CHECK(cinder::dev::platformNamed("Android") == Platform::Android);
    CHECK_FALSE(cinder::dev::platformNamed("Amiga").has_value());
    CHECK_FALSE(cinder::dev::platformSupported(Platform::IOS));
    CHECK_FALSE(cinder::dev::platformSupported(Platform::Windows));
    const Platform host = cinder::dev::hostPlatform();
    CHECK(cinder::dev::platformSupported(host) == (host == Platform::MacOS || host == Platform::Linux));
    CHECK(cinder::dev::configurationNamed("Shipping") == Configuration::Shipping);
    CHECK(cinder::dev::configurationNamed("development") == Configuration::Development);
}

TEST_CASE("targetPlatforms narrows what a project packages for, and empty means all") {
    const Platform host = cinder::dev::hostPlatform();
    const Packager open("/nowhere/Game", {}, tools());
    CHECK(open.targets(Platform::Android));
    CHECK(open.canPackage(host) == cinder::dev::platformSupported(host));
    CHECK(open.unavailableReason(Platform::IOS) == "iOS is not supported yet");

    const Packager mobile("/nowhere/Game", {"iOS", "Android"}, tools());
    CHECK_FALSE(mobile.targets(host));
    CHECK_FALSE(mobile.canPackage(host));
    CHECK(mobile.outputDir(Platform::Android) == std::filesystem::path("/nowhere/Game/Saved/Builds/Android/Game"));
}

TEST_CASE("the editor runs the packager binary with the platform and configuration") {
    const std::vector<std::string> arguments =
            cinder::dev::packageArguments(tools(), Platform::Linux, Configuration::Shipping, "/p/My Game", "/p/out's");
    CHECK(arguments == std::vector<std::string>{"/b/packager", "/p/My Game", "--platform", "Linux", "--config",
                                                "Shipping", "--out", "/p/out's"});
}

TEST_CASE("starting an unavailable platform does nothing") {
    Packager packager("/nowhere/Game", {}, tools());
    CHECK_FALSE(packager.start(Platform::IOS));
    CHECK_FALSE(packager.busy());
    packager.update();
}

TEST_CASE("staging copies the player, engine data and project, but never Source or Saved") {
    const Project project("layout", R"({"fileVersion": 1, "engineAssociation": "", "targetPlatforms": [")" + HOST + R"("]})");
    cinder::dev::stage(project.request());
    const std::filesystem::path out = project.root.parent_path() / "out";

    CHECK(std::filesystem::exists(out / "player"));
    CHECK((std::filesystem::status(out / "player").permissions() & std::filesystem::perms::owner_exec)
          != std::filesystem::perms::none);
    CHECK(std::filesystem::exists(out / "Engine" / "Content" / "Fonts" / "Roboto.ttf"));
    CHECK(std::filesystem::exists(out / "Project" / "Game.cinder"));
    CHECK(std::filesystem::exists(out / "Project" / "Config" / "Game.ini"));
    CHECK(std::filesystem::exists(out / "Project" / "Content" / "Scenes" / "main.scene"));
    CHECK_FALSE(std::filesystem::exists(out / "Project" / "Source"));
    CHECK_FALSE(std::filesystem::exists(out / "Project" / "Saved"));
    CHECK_FALSE(std::filesystem::exists(out / "Project" / "Binaries"));
    CHECK_FALSE(std::filesystem::exists(out / "Engine" / "Shaders" / "sprite.dxil"));
    if (HOST == "macOS") CHECK(std::filesystem::exists(out / "Engine" / "Shaders" / "sprite.metallib"));
    CHECK(std::filesystem::exists(out / "Engine" / "Shaders" / "sprite.spv"));
}

TEST_CASE("staging refuses a platform the project does not target") {
    const Project project("refuse", R"({"fileVersion": 1, "engineAssociation": "", "targetPlatforms": ["iOS"]})");
    CHECK_THROWS_AS(cinder::dev::stage(project.request()), std::runtime_error);
}

TEST_CASE("build steps run in the project folder with $(StageDir) expanded, and a failing one stops") {
    const Project project("steps", R"({"fileVersion": 1, "engineAssociation": "",
        "preBuildSteps": {")" + HOST + R"(": ["echo pre > pre.txt"]},
        "postBuildSteps": {")" + HOST + R"(": ["touch '$(StageDir)/post.txt'"]}})");
    cinder::dev::stage(project.request());
    CHECK(std::filesystem::exists(project.root / "pre.txt"));
    CHECK(std::filesystem::exists(project.root.parent_path() / "out" / "post.txt"));

    const Project failing("failing", R"({"fileVersion": 1, "engineAssociation": "",
        "preBuildSteps": {")" + HOST + R"(": ["false"]}})");
    CHECK_THROWS_AS(cinder::dev::stage(failing.request()), std::runtime_error);
    CHECK_FALSE(std::filesystem::exists(failing.root.parent_path() / "out"));
}

TEST_CASE("cancelling stops the packager's whole process group, and so does destroying the packager") {
    const Platform host = cinder::dev::hostPlatform();
    if (!cinder::dev::platformSupported(host)) return;

    const std::filesystem::path dir = testing::uniqueTempDir("cinder_packager_test");
    const std::filesystem::path slow = dir / "packager";
    Project::write(slow, "#!/bin/sh\nsleep 30 &\nwait\n");
    std::filesystem::permissions(slow, std::filesystem::perms::owner_all);

    const auto settle = [](const Packager& packager) {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (packager.busy() && std::chrono::steady_clock::now() < deadline) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    };

    Packager packager(dir / "Game", {}, PackageTools{slow});
    REQUIRE(packager.start(host));
    CHECK(packager.busy());
    CHECK(packager.current() == host);
    packager.cancel();
    settle(packager);
    CHECK_FALSE(packager.busy());
    packager.update();

    const auto before = std::chrono::steady_clock::now();
    {
        Packager doomed(dir / "Game", {}, PackageTools{slow});
        REQUIRE(doomed.start(host));
    }
    CHECK(std::chrono::steady_clock::now() - before < std::chrono::seconds(20));
    std::filesystem::remove_all(dir);
}

TEST_CASE("hot reload: a Shipping package compiles the project's C++, strips it and keeps its symbols") {
    const cinder::dev::GameBuild build{CINDER_CBT, CINDER_OUTPUT_ROOT, CINDER_CONFIG,
                                       std::string(cinder::dev::GameBuild::defaultBackend())};
    std::string error;

    const Project project("module", R"({"fileVersion": 1, "engineAssociation": ""})");
    Project::write(project.root / "Source" / "Mover.cpp", R"(#include "game/Game.hpp"
class Mover final : public cinder::scene::Spatial {
public:
    CINDER_NODE(Mover, cinder::scene::Spatial) {}
};
CINDER_REGISTER(Mover);
)");
    StageRequest request = project.request(Configuration::Shipping);
    request.build = build;
    request.player = build.binaries() / "player";
    cinder::dev::stage(request);

    const std::string file = "Game" + std::string(cinder::core::GameModule::extension());
    const std::filesystem::path shipped = request.out / "Project" / "Binaries" / HOST / file;
    const std::filesystem::path symbols = project.root / "Saved" / "Symbols" / HOST / file;
    REQUIRE(std::filesystem::exists(shipped));
    REQUIRE(std::filesystem::exists(symbols));
    CHECK(std::filesystem::file_size(shipped) < std::filesystem::file_size(symbols));
    CHECK(std::filesystem::exists(project.root / "Saved" / "Symbols" / HOST / "player"));
    CHECK(std::filesystem::file_size(request.out / "player") < std::filesystem::file_size(request.player));

    cinder::scene::NodeTypes types;
    std::unique_ptr<cinder::core::GameModule> module = cinder::core::GameModule::open(shipped, types, error);
    REQUIRE_MESSAGE(module != nullptr, error);
    module->enlist();
    CHECK(module->classes() == std::vector<std::string>{"Mover"});
}
