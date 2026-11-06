#include <doctest/doctest.h>

#include "core/ProjectConfig.hpp"
#include "core/ProjectDescriptor.hpp"
#include "platform/Log.hpp"

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

using cinder::core::ProjectConfig;
using cinder::core::ProjectDescriptor;

namespace {

const std::string MINIMAL = "{ \"fileVersion\": 1 }";

struct Folder {
    std::filesystem::path root = std::filesystem::temp_directory_path() / "cinder_project_test";

    Folder() {
        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root);
    }

    ~Folder() { std::filesystem::remove_all(root); }

    void write(const std::filesystem::path& relative, const std::string& text) const {
        const std::filesystem::path path = root / relative;
        std::filesystem::create_directories(path.parent_path());
        std::ofstream(path, std::ios::binary) << text;
    }
};

struct Sink {
    int lines = 0;

    Sink() {
        cinder::platform::setLogSink([this](cinder::platform::LogLevel, std::string_view) { lines++; });
    }

    ~Sink() { cinder::platform::setLogSink(nullptr); }

    Sink(const Sink&) = delete;
    Sink& operator=(const Sink&) = delete;
};

}

TEST_CASE("a project opens from its folder or its .cinder file") {
    Folder f;
    f.write("Game.cinder", MINIMAL);

    CHECK(ProjectConfig::root(f.root) == std::filesystem::absolute(f.root));
    CHECK(ProjectConfig::root(f.root / "Game.cinder") == std::filesystem::absolute(f.root));
    CHECK_THROWS_AS(ProjectConfig::root(f.root / "Missing.cinder"), std::runtime_error);
}

TEST_CASE("a project needs exactly one .cinder file") {
    Folder f;
    CHECK_THROWS_AS(ProjectConfig::load(f.root), std::runtime_error);

    f.write("A.cinder", MINIMAL);
    CHECK_NOTHROW(ProjectConfig::load(f.root));

    f.write("B.cinder", MINIMAL);
    CHECK_THROWS_AS(ProjectConfig::load(f.root), std::runtime_error);
}

TEST_CASE("a descriptor must be a supported version of the right shape") {
    CHECK_THROWS_AS(ProjectDescriptor::parse("{}"), std::runtime_error);
    CHECK_THROWS_AS(ProjectDescriptor::parse("{ \"fileVersion\": 2 }"), std::runtime_error);
    CHECK_THROWS_AS(ProjectDescriptor::parse("[1]"), std::runtime_error);
    CHECK_THROWS_AS(ProjectDescriptor::parse("{ \"fileVersion\": 1, \"category\": 3 }"),
                    std::runtime_error);
    CHECK_THROWS_AS(ProjectDescriptor::parse("{ \"fileVersion\": 1, \"targetPlatforms\": \"macOS\" }"),
                    std::runtime_error);
    CHECK_THROWS_AS(ProjectDescriptor::parse("{ \"fileVersion\": 1, \"modules\": [{ \"type\": \"Runtime\" }] }"),
                    std::runtime_error);
    CHECK_THROWS_AS(ProjectDescriptor::parse("{ \"fileVersion\": 1, \"preBuildSteps\": { \"macOS\": [1] } }"),
                    std::runtime_error);
}

TEST_CASE("a load error names the .cinder file") {
    Folder f;
    f.write("Game.cinder", "{ \"fileVersion\": 1, \"category\": true }");

    std::string message;
    try {
        ProjectConfig::load(f.root);
    } catch (const std::runtime_error& e) {
        message = e.what();
    }
    CHECK(message == "Game.cinder: \"category\" must be a string");
}

TEST_CASE("an empty descriptor saves the four keys Unreal always writes") {
    CHECK(ProjectDescriptor::save(ProjectDescriptor())
          == "{\n\t\"fileVersion\": 1,\n\t\"engineAssociation\": \"\",\n\t\"category\": \"\",\n"
             "\t\"description\": \"\"\n}\n");
}

TEST_CASE("every descriptor field survives a save and a parse") {
    ProjectDescriptor full;
    full.engineAssociation = "0.1.0";
    full.category = "Games";
    full.description = "A \"quoted\" game";
    full.disableEnginePluginsByDefault = true;

    ProjectDescriptor::Module module;
    module.name = "Gameplay";
    module.loadingPhase = "PostEngineInit";
    module.additionalDependencies = {"Engine"};
    full.modules.push_back(module);

    ProjectDescriptor::Plugin plugin;
    plugin.name = "Physics";
    plugin.enabled = false;
    plugin.optional = true;
    plugin.description = "rigid bodies";
    plugin.platformAllowList = {"macOS"};
    plugin.platformDenyList = {"Windows"};
    plugin.targetAllowList = {"Editor"};
    plugin.targetDenyList = {"Player"};
    full.plugins.push_back(plugin);

    full.additionalRootDirectories = {"../Shared"};
    full.additionalPluginDirectories = {"../Plugins"};
    full.targetPlatforms = {"macOS", "Linux"};
    full.preBuildSteps["macOS"] = {"echo pre"};
    full.postBuildSteps["Linux"] = {"echo one", "echo two"};

    const std::string text = ProjectDescriptor::save(full);
    const ProjectDescriptor back = ProjectDescriptor::parse(text);

    CHECK(ProjectDescriptor::save(back) == text);
    CHECK(back.description == "A \"quoted\" game");
    CHECK(back.disableEnginePluginsByDefault);
    REQUIRE(back.modules.size() == 1);
    CHECK(back.modules[0].loadingPhase == "PostEngineInit");
    CHECK(back.modules[0].additionalDependencies.size() == 1);
    REQUIRE(back.plugins.size() == 1);
    CHECK_FALSE(back.plugins[0].enabled);
    CHECK(back.plugins[0].optional);
    CHECK(back.plugins[0].targetDenyList.at(0) == "Player");
    CHECK(back.targetPlatforms.size() == 2);
    CHECK(back.postBuildSteps.at("Linux").at(1) == "echo two");
}

TEST_CASE("a project made with another engine version warns") {
    Folder f;
    Sink sink;

    f.write("Game.cinder", "{ \"fileVersion\": 1, \"engineAssociation\": \"99.0.0\" }");
    CHECK(ProjectConfig::load(f.root).descriptor.engineAssociation == "99.0.0");
    CHECK(sink.lines == 1);

    f.write("Game.cinder", "{ \"fileVersion\": 1, \"engineAssociation\": \""
                                   + std::string(ProjectDescriptor::engineVersion()) + "\" }");
    ProjectConfig::load(f.root);
    f.write("Game.cinder", MINIMAL);
    ProjectConfig::load(f.root);
    CHECK(sink.lines == 1);
}

TEST_CASE("settings come from Config/Game.ini and default without it") {
    Folder f;
    f.write("Game.cinder", MINIMAL);
    CHECK(ProjectConfig::load(f.root).title == "Cinder");

    f.write("Config/Game.ini",
            "[Game]\ntitle=Mine\nstartScene=Levels/one.scene\n\n[Window]\nheight=360\n");
    const ProjectConfig config = ProjectConfig::load(f.root);
    CHECK(config.title == "Mine");
    CHECK(config.startScene == "Levels/one.scene");
    CHECK(config.width == 1280);
    CHECK(config.height == 360);
    CHECK(config.fixedHz == 60);
}

TEST_CASE("gravity comes from Config/Game.ini") {
    Folder f;
    f.write("Game.cinder", MINIMAL);
    CHECK(ProjectConfig::load(f.root).gravity.y == -9.81f);

    f.write("Config/Game.ini", "[Physics]\ngravity=0 -20 0\n");
    const ProjectConfig config = ProjectConfig::load(f.root);
    CHECK(config.gravity.y == -20.0f);
    CHECK(ProjectConfig::save(config) == "[Physics]\ngravity=0 -20 0\n");
}

TEST_CASE("a non-positive fixedHz in Game.ini is rejected") {
    Folder f;
    f.write("Game.cinder", MINIMAL);
    f.write("Config/Game.ini", "[Game]\nfixedHz=0\n");
    CHECK_THROWS_AS(ProjectConfig::load(f.root), std::runtime_error);
}

TEST_CASE("a saved config writes only what differs from the defaults") {
    CHECK(ProjectConfig::save(ProjectConfig()).empty());

    ProjectConfig config;
    config.title = "Mine";
    config.height = 360;
    CHECK(ProjectConfig::save(config) == "[Game]\ntitle=Mine\n\n[Window]\nheight=360\n");
}
