#include <doctest/doctest.h>

#include "test_support.hpp"

#include "Dev/AssetCatalog.hpp"
#include "Dev/SceneDocument.hpp"
#include "Platform/Assets.hpp"
#include "Platform/Files.hpp"

#include <filesystem>
#include <string>
#include <vector>

using cinder::Dev::AssetCatalog;
using cinder::Dev::AssetEntry;
using cinder::Dev::AssetKind;

namespace {

struct Content {
    std::filesystem::path root = testing::uniqueTempDir("cinder_asset_catalog_test");

    Content() {
        std::filesystem::create_directories(root / "Content" / "Scenes" / "Levels");
        std::filesystem::create_directories(root / "Content" / "Prefabs");
        std::filesystem::create_directories(root / "Content" / "Textures");
        std::filesystem::create_directories(root / "Content" / ".hidden");
        cinder::Platform::setProjectRoot(root);
        write("Scenes/main.scene");
        write("Scenes/Levels/b.scene");
        write("Prefabs/ball.prefab");
        write("Textures/Box.PNG");
        write("Textures/notes.txt");
        write(".hidden/secret.scene");
        write(".DS_Store");
    }

    ~Content() { std::filesystem::remove_all(root); }

    void write(const std::string& name) const { cinder::Platform::writeTextFile(root / "Content" / name, "x"); }

    std::vector<std::string> names(const AssetCatalog& catalog, const std::vector<std::uint64_t>& ids) const {
        std::vector<std::string> out;
        for (const std::uint64_t id : ids) {
            const AssetEntry* entry = catalog.byId(id);
            out.push_back(entry != nullptr ? entry->path : std::string("?"));
        }
        return out;
    }
};

}

TEST_CASE("the asset catalog sorts Content into folders and assets by kind, skipping hidden files") {
    Content content;
    AssetCatalog catalog;
    catalog.refresh();

    CHECK(content.names(catalog, catalog.subfolders(AssetCatalog::ROOT))
          == std::vector<std::string>{"Prefabs", "Scenes", "Textures"});
    CHECK(content.names(catalog, catalog.contents(AssetCatalog::idOf("Scenes")))
          == std::vector<std::string>{"Scenes/Levels", "Scenes/main.scene"});
    CHECK(catalog.byPath("Textures/Box.PNG")->kind == AssetKind::Texture);
    CHECK(catalog.byPath("Textures/notes.txt")->kind == AssetKind::Other);
    CHECK(catalog.byPath("Prefabs/ball.prefab")->kind == AssetKind::Prefab);
    CHECK(catalog.byPath("Scenes/Levels")->kind == AssetKind::Folder);
    CHECK(catalog.byPath("Scenes/Levels/b.scene")->parent == AssetCatalog::idOf("Scenes/Levels"));
    CHECK(catalog.byPath(".hidden/secret.scene") == nullptr);
    CHECK(catalog.byPath(".DS_Store") == nullptr);
    CHECK(catalog.byId(AssetCatalog::ROOT)->name == "Content");
    CHECK(catalog.list(AssetKind::Scene) == std::vector<std::string>{"Scenes/Levels/b.scene", "Scenes/main.scene"});
    CHECK(content.names(catalog, catalog.search("MAIN")) == std::vector<std::string>{"Scenes/main.scene"});
}

TEST_CASE("an asset keeps its id across a rescan, and a rescan only counts as a change when something changed") {
    Content content;
    AssetCatalog catalog;
    catalog.refresh();
    const std::uint64_t ball = catalog.byPath("Prefabs/ball.prefab")->id;
    const std::uint64_t revision = catalog.revision();

    catalog.refresh();
    CHECK(catalog.revision() == revision);
    CHECK(catalog.byPath("Prefabs/ball.prefab")->id == ball);

    content.write("Prefabs/crate.prefab");
    catalog.refresh();
    CHECK(catalog.revision() == revision + 1);
    CHECK(catalog.byPath("Prefabs/ball.prefab")->id == ball);
    CHECK(catalog.byPath("Prefabs/crate.prefab") != nullptr);
}

TEST_CASE("SceneDocument lists scenes and prefabs through the same scan") {
    Content content;
    CHECK(cinder::Dev::SceneDocument::listScenes()
          == std::vector<std::string>{"Prefabs/ball.prefab", "Scenes/Levels/b.scene", "Scenes/main.scene"});
    CHECK(cinder::Dev::SceneDocument::listPrefabs() == std::vector<std::string>{"Prefabs/ball.prefab"});
}

TEST_CASE("New Folder picks a name nobody has, in the folder asked for") {
    Content content;
    AssetCatalog catalog;
    catalog.refresh();
    CHECK(catalog.createFolder("Textures") == std::optional<std::string>("Textures/New Folder"));
    CHECK(catalog.createFolder("Textures") == std::optional<std::string>("Textures/New Folder 2"));
    CHECK(catalog.createFolder("") == std::optional<std::string>("New Folder"));
    CHECK(catalog.byPath("Textures/New Folder 2")->kind == AssetKind::Folder);
    CHECK_FALSE(catalog.createFolder("../outside").has_value());
    CHECK_FALSE(catalog.createFolder("Textures/missing").has_value());
}

TEST_CASE("delete removes a file or a folder inside Content, and refuses the open scene and anything outside") {
    Content content;
    AssetCatalog catalog;
    catalog.refresh();
    const std::filesystem::path open = content.root / "Content" / "Scenes" / "main.scene";
    std::string error;

    CHECK(catalog.remove("Textures/notes.txt", open, &error));
    CHECK_FALSE(std::filesystem::exists(content.root / "Content" / "Textures" / "notes.txt"));
    CHECK(catalog.byPath("Textures/notes.txt") == nullptr);

    CHECK_FALSE(catalog.remove("Scenes/main.scene", open, &error));
    CHECK(error == "The open scene cannot be deleted");
    CHECK_FALSE(catalog.remove("Scenes", open, &error));
    CHECK(std::filesystem::exists(open));

    CHECK_FALSE(catalog.remove("../outside", open, &error));
    CHECK_FALSE(catalog.remove("/etc/hosts", open, &error));
    CHECK_FALSE(catalog.remove("", open, &error));
    CHECK_FALSE(catalog.remove("Textures/gone.png", open, &error));

    CHECK(catalog.remove("Scenes/Levels", open, &error));
    CHECK_FALSE(std::filesystem::exists(content.root / "Content" / "Scenes" / "Levels"));
    CHECK(catalog.byPath("Scenes/Levels/b.scene") == nullptr);
}
