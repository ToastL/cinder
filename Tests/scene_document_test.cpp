#include <doctest/doctest.h>

#include "test_support.hpp"

#include "Components/Actors.hpp"
#include "Dev/SceneDocument.hpp"
#include "Platform/Assets.hpp"
#include "Platform/Files.hpp"
#include "Serial/SceneCodec.hpp"

#include <filesystem>
#include <string>
#include <vector>

using cinder::Dev::SceneDocument;
using cinder::Serial::SceneCodec;

namespace {

struct Project : testing::EditorFixture {
    std::filesystem::path root = testing::uniqueTempDir("cinder_scene_document_test");
    SceneDocument document{scene, history, selection, {}};
    int switches = 0;

    Project() {
        std::filesystem::create_directories(root / "Content" / "Scenes" / "Levels");
        std::filesystem::create_directories(root / "Content" / "Textures");
        cinder::Platform::setProjectRoot(root);
        document.setOnSwitched([this] { ++switches; });
    }

    ~Project() { std::filesystem::remove_all(root); }

    std::filesystem::path write(const std::string& name, const std::string& actor) {
        cinder::Scene::ClassRegistry otherTypes;
        cinder::Components::registerBuiltins(otherTypes);
        cinder::Scene::Scene other(otherTypes);
        other.spawn("Actor")->setName(actor);
        const std::filesystem::path path = root / "Content" / name;
        cinder::Platform::writeTextFile(path, SceneCodec::save(other));
        return path;
    }
};

}

TEST_CASE("listScenes finds every scene under Content, nested ones included, sorted") {
    Project project;
    project.write("Scenes/main.scene", "A");
    project.write("Scenes/Levels/b.scene", "B");
    project.write("Scenes/Levels/a.scene", "C");
    cinder::Platform::writeTextFile(project.root / "Content" / "Textures" / "notes.txt", "x");

    CHECK(SceneDocument::listScenes()
          == std::vector<std::string>{"Scenes/Levels/a.scene", "Scenes/Levels/b.scene", "Scenes/main.scene"});
}

TEST_CASE("resolve adds the extension and refuses names outside Content") {
    Project project;
    const SceneDocument::Resolved plain = SceneDocument::resolve("  Scenes/level  ");
    CHECK(plain.error.empty());
    CHECK(plain.path == project.root / "Content" / "Scenes" / "level.scene");
    CHECK_FALSE(plain.exists);

    CHECK(SceneDocument::resolve("Scenes/level.scene").path == plain.path);
    CHECK_FALSE(SceneDocument::resolve("").error.empty());
    CHECK_FALSE(SceneDocument::resolve("   ").error.empty());
    CHECK_FALSE(SceneDocument::resolve("../escape").error.empty());
    CHECK_FALSE(SceneDocument::resolve("/tmp/escape").error.empty());
    CHECK_FALSE(SceneDocument::resolve("Scenes/").error.empty());

    project.write("Scenes/main.scene", "A");
    CHECK(SceneDocument::resolve("Scenes/main").exists);
}

TEST_CASE("the display name is relative to Content, or the file name outside it") {
    Project project;
    CHECK(project.document.displayName() == "Untitled");
    CHECK(SceneDocument::nameOf(project.root / "Content" / "Scenes" / "main.scene") == "Scenes/main.scene");
    CHECK(SceneDocument::nameOf(project.root / "Elsewhere" / "saved.scene") == "saved.scene");
}

TEST_CASE("opening a scene replaces the world, resets history and clears the selection") {
    Project project;
    const std::filesystem::path path = project.write("Scenes/level.scene", "Loaded");
    cinder::Scene::Actor* old = project.scene.spawn("Actor");
    project.history.reset();
    project.selection.select(old->id());
    project.history.touch("Rename", std::nullopt);
    old->setName("Edited");
    project.history.settle(false);
    REQUIRE(project.history.dirty());

    REQUIRE(project.document.open(path));
    CHECK(project.scene.find("Loaded") != nullptr);
    CHECK(project.scene.find("Edited") == nullptr);
    CHECK_FALSE(project.history.dirty());
    CHECK_FALSE(project.history.canUndo());
    CHECK_FALSE(project.selection.id().has_value());
    CHECK(project.document.path() == path);
    CHECK(project.document.displayName() == "Scenes/level.scene");
    CHECK(project.switches == 1);
}

TEST_CASE("a scene that fails to load leaves the world and the document as they were") {
    Project project;
    const std::filesystem::path good = project.write("Scenes/good.scene", "Good");
    REQUIRE(project.document.open(good));
    const std::filesystem::path bad = project.root / "Content" / "Scenes" / "bad.scene";
    cinder::Platform::writeTextFile(bad, "version 5\nactors {\n");

    CHECK_FALSE(project.document.open(bad));
    CHECK_FALSE(project.document.open(project.root / "Content" / "Scenes" / "missing.scene"));
    CHECK(project.scene.find("Good") != nullptr);
    CHECK(project.document.path() == good);
    CHECK(project.switches == 1);
}

TEST_CASE("a new scene holds one camera and is untitled and clean") {
    Project project;
    project.scene.spawn("Actor")->setName("Old");
    project.document.create();

    CHECK(project.document.untitled());
    CHECK(project.scene.find("Old") == nullptr);
    REQUIRE(project.scene.roots().size() == 1);
    CHECK(dynamic_cast<cinder::Components::CameraActor*>(project.scene.roots().front()) != nullptr);
    CHECK_FALSE(project.history.dirty());
    CHECK_FALSE(project.document.save());
}

TEST_CASE("Save As writes the scene, adopts the path and clears dirty") {
    Project project;
    project.document.create();
    project.history.touch("Add", std::nullopt);
    project.scene.spawn("Actor")->setName("Added");
    project.history.settle(false);
    REQUIRE(project.history.dirty());

    const std::filesystem::path path = SceneDocument::resolve("Scenes/Levels/fresh").path;
    REQUIRE(project.document.saveAs(path));
    CHECK(project.document.path() == path);
    CHECK_FALSE(project.history.dirty());
    CHECK(cinder::Platform::readTextFile(path) == SceneCodec::save(project.scene));
    CHECK(project.switches == 1);
}

TEST_CASE("a requested switch waits for applyPending") {
    Project project;
    const std::filesystem::path path = project.write("Scenes/level.scene", "Loaded");
    project.document.requestOpen(path);
    CHECK(project.document.pending());
    CHECK(project.scene.find("Loaded") == nullptr);
    CHECK(project.document.applyPending());
    CHECK_FALSE(project.document.pending());
    CHECK(project.scene.find("Loaded") != nullptr);

    project.document.requestNew();
    CHECK(project.document.applyPending());
    CHECK(project.document.untitled());
    CHECK_FALSE(project.document.applyPending());
}
