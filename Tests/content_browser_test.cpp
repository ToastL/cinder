#include <doctest/doctest.h>

#include "test_support.hpp"

#include "ui_harness.hpp"

#include "Components/Actors.hpp"
#include "Components/SpriteComponent.hpp"
#include "Dev/AssetCatalog.hpp"
#include "Dev/ContentActions.hpp"
#include "Dev/Panels/ContentBrowser.hpp"
#include "Dev/Panels/Details.hpp"
#include "Dev/Panels/EditorStyle.hpp"
#include "Dev/Panels/Outliner.hpp"
#include "Platform/Assets.hpp"
#include "Platform/Files.hpp"
#include "Scene/Actor.hpp"
#include "Serial/SceneCodec.hpp"
#include "UI/Widgets/BoxPanel.hpp"
#include "UI/Widgets/Button.hpp"
#include "UI/Widgets/DropTarget.hpp"
#include "UI/Widgets/Label.hpp"
#include "UI/Widgets/TileView.hpp"
#include "UI/Widgets/TreeView.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

using namespace cinder::UI;
using cinder::Dev::AssetCatalog;
using cinder::Dev::ContentActions;
using cinder::Dev::Panels::ContentBrowser;
using cinder::Dev::Panels::Details;
using cinder::Dev::Panels::Outliner;
using cinder::Scene::Actor;
using uitest::Harness;
namespace keys = cinder::Platform::keys;

namespace {

struct Project : testing::EditorFixture {
    std::filesystem::path root = testing::uniqueTempDir("cinder_content_browser_test");
    AssetCatalog catalog;

    Project() {
        std::filesystem::create_directories(root / "Content" / "Scenes");
        std::filesystem::create_directories(root / "Content" / "Prefabs");
        std::filesystem::create_directories(root / "Content" / "Textures");
        cinder::Platform::setProjectRoot(root);
        cinder::Platform::writeTextFile(root / "Content" / "Scenes" / "main.scene", "version 6\n");
        cinder::Platform::writeTextFile(root / "Content" / "Textures" / "flag.png", "not really a png");

        cinder::Scene::Scene prefab(types);
        prefab.spawn("StaticMeshActor")->setName("Crate");
        cinder::Platform::writeTextFile(root / "Content" / "Prefabs" / "crate.prefab",
                                        cinder::Serial::SceneCodec::save(prefab));

        Actor* group = scene.spawn("Actor");
        group->setName("Props");
        scene.spawn("SpriteActor", group)->setName("Flag");
        history.reset();
    }

    ~Project() { std::filesystem::remove_all(root); }
};

template <class T>
T* find(Widget* widget) {
    if (widget == nullptr) return nullptr;
    if (auto* found = dynamic_cast<T*>(widget)) return found;
    for (int i = 0; i < widget->childCount(); ++i) {
        if (T* found = find<T>(widget->childAt(i))) return found;
    }
    return nullptr;
}

bool labelled(Widget* widget, const std::string& text) {
    if (auto* label = dynamic_cast<Label*>(widget); label != nullptr && label->text() == text) return true;
    for (int i = 0; i < widget->childCount(); ++i) {
        if (labelled(widget->childAt(i), text)) return true;
    }
    return false;
}

Button* buttonNamed(Widget* widget, const std::string& text) {
    if (widget == nullptr) return nullptr;
    if (auto* button = dynamic_cast<Button*>(widget); button != nullptr && labelled(button, text)) return button;
    for (int i = 0; i < widget->childCount(); ++i) {
        if (Button* found = buttonNamed(widget->childAt(i), text)) return found;
    }
    return nullptr;
}

ItemId id(const std::string& path) { return AssetCatalog::idOf(path); }

void dragTile(Harness& ui, const std::shared_ptr<TileView>& tiles, const std::string& path, glm::vec2 to) {
    const Rect tile = ui.rectOf(tiles->tileFor(id(path)));
    ui.move(tile.center().x, tile.center().y);
    ui.press();
    ui.move(tile.center().x + 12.0f, tile.center().y + 4.0f);
    REQUIRE(ui.app.dragDrop());
    ui.move(to.x, to.y);
    ui.release();
    ui.frame();
}

}

TEST_CASE("the Content Browser lists a folder's subfolders then its assets, and navigates by tile and crumb") {
    Project project;
    Harness ui(nullptr);
    ContentBrowser browser(project.catalog, ui.app);
    ui.app.setTheme(cinder::Dev::Panels::editorTheme());
    ui.app.setRoot(browser.widget());
    ui.size = {900.0f, 400.0f};
    browser.update();
    ui.frame();
    ui.frame();

    const std::shared_ptr<TileView> tiles = browser.tiles();
    REQUIRE(tiles->itemCount() == 3);
    CHECK(tiles->itemAt(0) == id("Prefabs"));
    CHECK(tiles->itemAt(1) == id("Scenes"));
    CHECK(tiles->itemAt(2) == id("Textures"));
    CHECK(browser.crumbs()->slotCount() == 1);

    const Rect prefabs = ui.rectOf(tiles->tileFor(id("Prefabs")));
    ui.click(prefabs.center().x, prefabs.center().y);
    ui.click(prefabs.center().x, prefabs.center().y);
    browser.update();
    ui.frame();
    CHECK(browser.folder() == "Prefabs");
    REQUIRE(tiles->itemCount() == 1);
    CHECK(tiles->itemAt(0) == id("Prefabs/crate.prefab"));
    CHECK(browser.folders()->isSelected(id("Prefabs")));
    CHECK(browser.crumbs()->slotCount() == 3);

    Button* top = buttonNamed(browser.crumbs().get(), "Content");
    REQUIRE(top != nullptr);
    const Rect crumb = ui.app.grid().geometryOf(top).value_or(Geometry{}).rect();
    ui.click(crumb.center().x, crumb.center().y);
    browser.update();
    ui.frame();
    CHECK(browser.folder().empty());
    CHECK(tiles->itemCount() == 3);

    browser.setFilter("fla");
    ui.frame();
    REQUIRE(tiles->itemCount() == 1);
    CHECK(tiles->itemAt(0) == id("Textures/flag.png"));
}

TEST_CASE("the Content Browser opens a scene on a double click and deletes through the confirm prompt") {
    Project project;
    Harness ui(nullptr);
    ContentBrowser browser(project.catalog, ui.app);
    std::optional<std::filesystem::path> opened;
    std::vector<std::string> confirmed;
    ContentActions actions;
    actions.openScene = [&](const std::filesystem::path& path) { opened = path; };
    actions.confirm = [&](std::string, std::string, std::string label, std::function<void()> action) {
        confirmed.push_back(label);
        action();
    };
    actions.openPath = [&] { return project.root / "Content" / "Scenes" / "main.scene"; };
    browser.setActions(actions);
    ui.app.setTheme(cinder::Dev::Panels::editorTheme());
    ui.app.setRoot(browser.widget());
    ui.size = {900.0f, 400.0f};
    browser.navigate("Scenes");
    browser.update();
    ui.frame();
    ui.frame();

    const std::shared_ptr<TileView> tiles = browser.tiles();
    const Rect scene = ui.rectOf(tiles->tileFor(id("Scenes/main.scene")));
    ui.click(scene.center().x, scene.center().y);
    ui.click(scene.center().x, scene.center().y);
    REQUIRE(opened.has_value());
    CHECK(*opened == project.root / "Content" / "Scenes" / "main.scene");

    ui.key(keys::DELETE_KEY);
    CHECK(confirmed == std::vector<std::string>{"Delete"});
    CHECK(std::filesystem::exists(project.root / "Content" / "Scenes" / "main.scene"));

    browser.reveal("Textures/flag.png");
    browser.update();
    ui.frame();
    CHECK(browser.folder() == "Textures");
    CHECK(browser.selectedPath() == std::optional<std::string>("Textures/flag.png"));
    ui.app.setFocus(tiles);
    ui.key(keys::BACKSPACE);
    CHECK(confirmed.size() == 2);
    CHECK_FALSE(std::filesystem::exists(project.root / "Content" / "Textures" / "flag.png"));
    browser.update();
    ui.frame();
    CHECK(tiles->itemCount() == 0);
}

TEST_CASE("dropping a prefab tile on an Outliner row places an instance under that actor, as one undo step") {
    Project project;
    Harness ui(nullptr);
    ContentBrowser browser(project.catalog, ui.app);
    Outliner outliner(project.selection, project.history, project.scene, ui.app);
    ui.app.setTheme(cinder::Dev::Panels::editorTheme());
    ui.app.setRoot(make<HorizontalBox>() + HorizontalBox::slot().fill(1.0f)[outliner.widget()]
                   + HorizontalBox::slot().fill(2.0f)[browser.widget()]);
    ui.size = {1200.0f, 400.0f};
    browser.navigate("Prefabs");
    browser.update();
    ui.frame();
    ui.frame();

    Actor& props = *project.scene.find("Props");
    const Rect row = ui.rectOf(outliner.tree()->rowFor(static_cast<ItemId>(props.id())));
    dragTile(ui, browser.tiles(), "Prefabs/crate.prefab", row.center());

    Actor* crate = project.scene.find("Crate");
    REQUIRE(crate != nullptr);
    CHECK(crate->attachParent() == &props);
    CHECK(crate->prefab() == "Prefabs/crate.prefab");
    CHECK(project.selection.id() == crate->id());
    project.history.settle(false);
    REQUIRE(project.history.canUndo());
    project.history.undo(project.selection);
    CHECK(project.scene.find("Crate") == nullptr);
}

TEST_CASE("a Details texture field takes a dropped texture tile, and Browse reveals what it names") {
    Project project;
    Harness ui(nullptr);
    ContentBrowser browser(project.catalog, ui.app);
    Details details(project.selection, project.history, project.scene, ui.app);
    std::optional<std::string> revealed;
    ContentActions actions;
    actions.reveal = [&](const std::string& path) { revealed = path; };
    details.setContentActions(actions);
    ui.app.setTheme(cinder::Dev::Panels::editorTheme());
    ui.app.setRoot(make<HorizontalBox>() + HorizontalBox::slot().fill(1.0f)[details.widget()]
                   + HorizontalBox::slot().fill(2.0f)[browser.widget()]);
    ui.size = {1200.0f, 900.0f};
    browser.navigate("Textures");
    project.selection.select(project.scene.find("Flag")->id());
    details.update();
    browser.update();
    ui.frame();
    ui.frame();

    DropTarget* field = find<DropTarget>(details.widget().get());
    REQUIRE(field != nullptr);
    Button* browse = buttonNamed(field, "Browse");
    REQUIRE(browse != nullptr);
    CHECK_FALSE(browse->isEnabled());

    const Rect target = ui.app.grid().geometryOf(field).value_or(Geometry{}).rect();
    dragTile(ui, browser.tiles(), "Textures/flag.png", target.center());

    auto* sprite = project.scene.find("Flag")->findComponent<cinder::Components::SpriteComponent>();
    REQUIRE(sprite != nullptr);
    CHECK(sprite->texture() == "Textures/flag.png");
    project.history.settle(false);
    CHECK(project.history.canUndo());

    details.update();
    ui.frame();
    browse = buttonNamed(find<DropTarget>(details.widget().get()), "Browse");
    REQUIRE(browse != nullptr);
    const Rect button = ui.app.grid().geometryOf(browse).value_or(Geometry{}).rect();
    ui.click(button.center().x, button.center().y);
    CHECK(revealed == std::optional<std::string>("Textures/flag.png"));
}
