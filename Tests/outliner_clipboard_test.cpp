#include <doctest/doctest.h>

#include "test_support.hpp"
#include "ui_harness.hpp"

#include "Dev/History.hpp"
#include "Dev/Selection.hpp"
#include "Dev/Panels/EditorStyle.hpp"
#include "Dev/Panels/Outliner.hpp"
#include "Dev/Panels/OutlinerClipboard.hpp"
#include "Scene/Actor.hpp"
#include "Scene/Scene.hpp"
#include "UI/Framework/PlatformHooks.hpp"
#include "UI/Widgets/TreeView.hpp"

#include <string>
#include <vector>

using namespace cinder::UI;
using cinder::Dev::Panels::Outliner;
using cinder::Dev::Panels::OutlinerClipboard;
using cinder::Scene::Actor;
using uitest::Harness;
namespace keys = cinder::Platform::keys;
namespace modifiers = cinder::Platform::modifiers;

namespace {

struct World : testing::EditorFixture {
    World() {
        Actor* box = scene.spawn("StaticMeshActor");
        box->setName("Box");
        Actor* group = scene.spawn("Actor");
        group->setName("Props");
        scene.spawn("SpriteActor", group)->setName("Flag");
        history.reset();
    }
};

}

TEST_CASE("Copy on an Outliner selection writes a cinder-clipboard payload") {
    World world;
    Harness ui(nullptr);
    Outliner outliner(world.selection, world.history, world.scene, ui.app);
    ui.app.setTheme(cinder::Dev::Panels::editorTheme());
    ui.app.setRoot(outliner.widget());
    ui.size = {320.0f, 400.0f};
    ui.frame();

    const int boxId = world.scene.find("Box")->id();
    const Rect row = ui.rectOf(outliner.tree()->rowFor(static_cast<ItemId>(boxId)));
    ui.click(row.center().x, row.center().y);
    ui.key(keys::letter('c'), modifiers::PRIMARY);
    ui.frame();

    const std::string payload = ui.platform.clipboard();
    CHECK(OutlinerClipboard::recognizes(payload));
    CHECK(world.scene.find("Box") != nullptr);
    CHECK_FALSE(world.history.canUndo());
}

TEST_CASE("Paste inserts a fresh copy as a sibling of the selection and selects it") {
    World world;
    Harness ui(nullptr);
    Outliner outliner(world.selection, world.history, world.scene, ui.app);
    ui.app.setTheme(cinder::Dev::Panels::editorTheme());
    ui.app.setRoot(outliner.widget());
    ui.size = {320.0f, 400.0f};
    ui.frame();

    const int boxId = world.scene.find("Box")->id();
    const Rect row = ui.rectOf(outliner.tree()->rowFor(static_cast<ItemId>(boxId)));
    ui.click(row.center().x, row.center().y);
    ui.key(keys::letter('c'), modifiers::PRIMARY);
    ui.frame();

    const std::size_t before = world.scene.roots().size();
    ui.key(keys::letter('v'), modifiers::PRIMARY);
    ui.frame();
    CHECK(world.scene.roots().size() == before + 1);

    const std::optional<int> selected = world.selection.id();
    REQUIRE(selected.has_value());
    CHECK(*selected != boxId);
    const Actor* pasted = world.scene.actor(*selected);
    REQUIRE(pasted != nullptr);
    CHECK(pasted->name() == "Box");
    CHECK(pasted->attachParent() == nullptr);

    world.history.settle(false);
    CHECK(world.history.canUndo());
    CHECK(world.history.undoLabel() == "Paste");
}

TEST_CASE("Cut copies then removes, and lands as one history step") {
    World world;
    Harness ui(nullptr);
    Outliner outliner(world.selection, world.history, world.scene, ui.app);
    ui.app.setTheme(cinder::Dev::Panels::editorTheme());
    ui.app.setRoot(outliner.widget());
    ui.size = {320.0f, 400.0f};
    ui.frame();

    const int boxId = world.scene.find("Box")->id();
    const Rect row = ui.rectOf(outliner.tree()->rowFor(static_cast<ItemId>(boxId)));
    ui.click(row.center().x, row.center().y);
    ui.key(keys::letter('x'), modifiers::PRIMARY);
    ui.frame();

    CHECK(world.scene.find("Box") == nullptr);
    CHECK(OutlinerClipboard::recognizes(ui.platform.clipboard()));

    world.history.settle(false);
    CHECK(world.history.canUndo());
    CHECK(world.history.undoLabel() == "Cut Box");

    ui.key(keys::letter('v'), modifiers::PRIMARY);
    ui.frame();
    REQUIRE(world.scene.find("Box") != nullptr);
}

TEST_CASE("Paste with a parent-attached selection lands as that actor's sibling") {
    World world;
    Harness ui(nullptr);
    Outliner outliner(world.selection, world.history, world.scene, ui.app);
    ui.app.setTheme(cinder::Dev::Panels::editorTheme());
    ui.app.setRoot(outliner.widget());
    ui.size = {320.0f, 400.0f};
    ui.frame();

    const int boxId = world.scene.find("Box")->id();
    const Rect boxRow = ui.rectOf(outliner.tree()->rowFor(static_cast<ItemId>(boxId)));
    ui.click(boxRow.center().x, boxRow.center().y);
    ui.key(keys::letter('c'), modifiers::PRIMARY);
    ui.frame();
    REQUIRE(OutlinerClipboard::recognizes(ui.platform.clipboard()));

    Actor* flag = world.scene.find("Flag");
    REQUIRE(flag != nullptr);
    Actor* props = flag->attachParent();
    REQUIRE(props != nullptr);

    world.selection.select(flag->id());
    ui.app.setFocus(outliner.tree());
    ui.frame();
    REQUIRE(ui.app.focused() == outliner.tree());

    const int flagId = flag->id();
    const int propsId = props->id();
    ui.key(keys::letter('v'), modifiers::PRIMARY);
    ui.frame();

    Actor* repropped = world.scene.actor(propsId);
    REQUIRE(repropped != nullptr);
    Actor* newChild = nullptr;
    for (const Actor* child : repropped->attachedActors()) {
        if (child->id() != flagId) newChild = world.scene.actor(child->id());
    }
    REQUIRE(newChild != nullptr);
    CHECK(newChild->name() == "Box");
    CHECK(world.selection.id() == newChild->id());
}

TEST_CASE("Paste with a foreign clipboard payload is a silent no-op") {
    World world;
    Harness ui(nullptr);
    Outliner outliner(world.selection, world.history, world.scene, ui.app);
    ui.app.setTheme(cinder::Dev::Panels::editorTheme());
    ui.app.setRoot(outliner.widget());
    ui.size = {320.0f, 400.0f};
    ui.frame();

    ui.platform.setClipboard("hello from somewhere else");

    const std::size_t rootsBefore = world.scene.roots().size();
    outliner.tree()->setSelection(std::nullopt);
    ui.app.setFocus(outliner.tree());
    ui.key(keys::letter('v'), modifiers::PRIMARY);
    ui.frame();

    CHECK(world.scene.roots().size() == rootsBefore);
    world.history.settle(false);
    CHECK_FALSE(world.history.canUndo());
}

TEST_CASE("OutlinerClipboard round-trips a subtree and remaps a Ref into the copy") {
    World world;
    Actor* box = world.scene.find("Box");
    REQUIRE(box != nullptr);

    const Actor* roots[] = {box};
    const std::string payload = OutlinerClipboard::encode(world.scene, roots);
    REQUIRE(OutlinerClipboard::recognizes(payload));

    const std::vector<Actor*> inserted = OutlinerClipboard::decode(payload, world.scene, nullptr);
    REQUIRE(inserted.size() == 1);
    CHECK(inserted.front()->name() == "Box");
    CHECK(inserted.front()->id() != box->id());
}
