#include <doctest/doctest.h>

#include "test_support.hpp"

#include "ui_harness.hpp"

#include "Dev/History.hpp"
#include "Dev/PrefabActions.hpp"
#include "Dev/Selection.hpp"
#include "Dev/Panels/Dialog.hpp"
#include "Dev/Panels/EditorStyle.hpp"
#include "Dev/Panels/Details.hpp"
#include "Dev/Panels/Outliner.hpp"
#include "Dev/Panels/ScenePicker.hpp"
#include "Platform/Assets.hpp"
#include "Platform/Files.hpp"
#include "Scene/Actor.hpp"
#include "Scene/ClassRegistry.hpp"
#include "Scene/SceneComponent.hpp"
#include "UI/Widgets/Button.hpp"
#include "Scene/PropValue.hpp"
#include "Scene/Ref.hpp"
#include "Scene/Scene.hpp"
#include "Scene/Transform.hpp"
#include "UI/Framework/Commands.hpp"
#include "UI/Widgets/BoxPanel.hpp"
#include "UI/Widgets/ComboBox.hpp"
#include "UI/Widgets/Label.hpp"
#include "UI/Widgets/Menu.hpp"
#include "UI/Widgets/SpinBox.hpp"
#include "UI/Widgets/TextField.hpp"
#include "UI/Widgets/TreeView.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

using namespace cinder::UI;
using cinder::Dev::History;
using cinder::Dev::Selection;
using cinder::Dev::Panels::Details;
using cinder::Dev::Panels::Outliner;
using cinder::Scene::Actor;
using cinder::Scene::PropValue;
using cinder::Scene::Scene;
using uitest::Harness;
namespace keys = cinder::Platform::keys;

namespace {

struct World : testing::EditorFixture {
    World() {
        Actor* box = scene.spawn("StaticMeshActor");
        box->setName("Box");
        box->setAttribute("phase", PropValue::number(2.0));
        Actor* group = scene.spawn("Actor");
        group->setName("Props");
        scene.spawn("SpriteActor", group)->setName("Flag");
        history.reset();
    }

    Actor& node(const std::string& name) { return *scene.find(name); }
};

}

TEST_CASE("the Outliner lists the actors, selects a row and deletes the selection with a key") {
    World world;
    Harness ui(nullptr);
    Outliner outliner(world.selection, world.history, world.scene, ui.app);
    ui.app.setTheme(cinder::Dev::Panels::editorTheme());
    ui.app.setRoot(outliner.widget());
    ui.size = {320.0f, 400.0f};
    ui.frame();

    const std::shared_ptr<TreeView> tree = outliner.tree();
    REQUIRE(tree->visibleCount() == 2);

    const auto box = static_cast<ItemId>(world.node("Box").id());
    const Rect row = ui.rectOf(tree->rowFor(box));
    ui.click(row.center().x, row.center().y);
    CHECK(world.selection.id() == world.node("Box").id());
    CHECK(tree->isSelected(box));
    CHECK(ui.app.focused() == tree);

    ui.key(keys::DELETE_KEY);
    ui.frame();
    CHECK(world.scene.find("Box") == nullptr);
    CHECK_FALSE(world.selection.id().has_value());
    CHECK(tree->visibleCount() == 1);
    world.history.settle(false);
    CHECK(world.history.canUndo());
}

TEST_CASE("the Outliner's filter flattens the tree to what matches") {
    World world;
    Harness ui(nullptr);
    Outliner outliner(world.selection, world.history, world.scene, ui.app);
    ui.app.setTheme(cinder::Dev::Panels::editorTheme());
    ui.app.setRoot(outliner.widget());
    ui.size = {320.0f, 400.0f};
    ui.frame();

    const std::shared_ptr<TreeView> tree = outliner.tree();
    CHECK(tree->visibleCount() == 2);
    CHECK(tree->hasChildren(static_cast<ItemId>(world.node("Props").id())));

    ui.click(200.0f, 14.0f);
    ui.type(U"fla");
    ui.frame();
    CHECK(tree->visibleCount() == 1);
    CHECK(tree->itemAt(0) == static_cast<ItemId>(world.node("Flag").id()));
    CHECK_FALSE(tree->hasChildren(static_cast<ItemId>(world.node("Props").id())));
}

TEST_CASE("dragging a row in the Outliner attaches the actor, and refuses a cycle") {
    World world;
    Harness ui(nullptr);
    Outliner outliner(world.selection, world.history, world.scene, ui.app);
    ui.app.setTheme(cinder::Dev::Panels::editorTheme());
    ui.app.setRoot(outliner.widget());
    ui.size = {320.0f, 400.0f};
    ui.frame();

    const std::shared_ptr<TreeView> tree = outliner.tree();
    const Rect box = ui.rectOf(tree->rowFor(static_cast<ItemId>(world.node("Box").id())));
    const Rect props = ui.rectOf(tree->rowFor(static_cast<ItemId>(world.node("Props").id())));
    ui.move(box.center().x, box.center().y);
    ui.press();
    ui.move(box.center().x + 12.0f, box.center().y + 4.0f);
    REQUIRE(ui.app.dragDrop());
    ui.move(props.center().x, props.center().y);
    ui.release();
    ui.frame();
    CHECK(world.node("Box").attachParent() == &world.node("Props"));
    CHECK(tree->isExpanded(static_cast<ItemId>(world.node("Props").id())));

    const Rect parent = ui.rectOf(tree->rowFor(static_cast<ItemId>(world.node("Props").id())));
    const Rect child = ui.rectOf(tree->rowFor(static_cast<ItemId>(world.node("Box").id())));
    ui.platform.advance(1.0);
    ui.move(parent.center().x, parent.center().y);
    ui.press();
    ui.move(parent.center().x + 12.0f, parent.center().y + 4.0f);
    REQUIRE(ui.app.dragDrop());
    ui.move(child.center().x, child.center().y);
    CHECK_FALSE(tree->dropZoneFor(static_cast<ItemId>(world.node("Box").id())));
    ui.release();
    ui.frame();
    CHECK(world.node("Props").attachParent() == nullptr);
}

TEST_CASE("Details edits the root component's transform through its definition and marks the scene dirty") {
    World world;
    Harness ui(nullptr);
    Details details(world.selection, world.history, world.scene, ui.app);
    ui.app.setTheme(cinder::Dev::Panels::editorTheme());
    ui.app.setRoot(details.widget());
    ui.size = {320.0f, 500.0f};
    ui.frame();

    world.selection.select(world.node("Box").id());
    details.update();
    ui.frame();

    std::shared_ptr<SpinBox> positionX;
    std::function<void(const std::shared_ptr<Widget>&)> walk = [&](const std::shared_ptr<Widget>& widget) {
        if (!widget || positionX) return;
        if (auto spin = std::dynamic_pointer_cast<SpinBox>(widget)) {
            if (spin->value() == 0.0 && !positionX) positionX = spin;
            return;
        }
        for (int i = 0; i < widget->childCount(); ++i) {
            if (Widget* child = widget->childAt(i)) {
                walk(child->shared_from_this());
                if (positionX) return;
            }
        }
    };
    walk(details.widget());
    REQUIRE(positionX);

    const Rect field = ui.rectOf(positionX);
    ui.move(field.center().x, field.center().y);
    ui.press();
    ui.move(field.center().x + 10.0f, field.center().y);
    ui.move(field.center().x + 20.0f, field.center().y);
    ui.release();
    CHECK(world.node("Box").transform()->position().x == doctest::Approx(1.0f));
    world.history.settle(ui.app.isInteracting());
    CHECK(world.history.canUndo());
    CHECK(world.history.dirty());
}

TEST_CASE("Details lists attributes, and rebuilds when one is added or removed") {
    World world;
    Harness ui(nullptr);
    Details details(world.selection, world.history, world.scene, ui.app);
    ui.app.setTheme(cinder::Dev::Panels::editorTheme());
    ui.app.setRoot(details.widget());
    ui.size = {320.0f, 500.0f};
    ui.frame();

    world.selection.select(world.node("Box").id());
    details.update();
    ui.frame();

    const auto labelled = [&](const std::string& text) {
        bool found = false;
        std::function<void(Widget*)> walk = [&](Widget* widget) {
            if (widget == nullptr || found) return;
            if (auto* label = dynamic_cast<Label*>(widget); label != nullptr && label->text() == text) {
                found = true;
                return;
            }
            for (int i = 0; i < widget->childCount(); ++i) walk(widget->childAt(i));
        };
        walk(details.widget().get());
        return found;
    };
    CHECK(labelled("phase"));
    CHECK_FALSE(labelled("speed"));

    world.node("Box").setAttribute("speed", PropValue::number(4.0));
    details.update();
    ui.frame();
    CHECK(labelled("speed"));

    world.node("Box").removeAttribute("phase");
    details.update();
    ui.frame();
    CHECK_FALSE(labelled("phase"));

    world.selection.clear();
    details.update();
    ui.frame();
    CHECK(labelled("Nothing selected"));
}

TEST_CASE("Escape in a Details text field cancels instead of writing") {
    World world;
    world.node("Box").setAttribute("tag", PropValue::text("hero"));
    world.history.reset();
    Harness ui(nullptr);
    Details details(world.selection, world.history, world.scene, ui.app);
    ui.app.setTheme(cinder::Dev::Panels::editorTheme());
    ui.app.setRoot(details.widget());
    ui.size = {320.0f, 500.0f};
    ui.frame();
    world.selection.select(world.node("Box").id());
    details.update();
    ui.frame();

    TextField* field = nullptr;
    std::function<void(Widget*)> walk = [&](Widget* widget) {
        if (widget == nullptr || field != nullptr) return;
        if (auto* text = dynamic_cast<TextField*>(widget); text != nullptr && text->text() == "hero") {
            field = text;
            return;
        }
        for (int i = 0; i < widget->childCount(); ++i) walk(widget->childAt(i));
    };
    walk(details.widget().get());
    REQUIRE(field != nullptr);

    const Rect rect = ui.app.grid().geometryOf(field).value_or(Geometry{}).rect();
    ui.click(rect.center().x, rect.center().y);
    ui.type(U"villain");
    ui.key(keys::ESCAPE);
    details.update();
    ui.frame();
    world.history.settle(ui.app.isInteracting());

    REQUIRE(world.node("Box").attribute("tag") != nullptr);
    CHECK(world.node("Box").attribute("tag")->as<std::string>() == "hero");
    CHECK_FALSE(world.history.dirty());
}

TEST_CASE("the Outliner highlights the owner when a component is selected") {
    World world;
    Harness ui(nullptr);
    Outliner outliner(world.selection, world.history, world.scene, ui.app);
    ui.app.setTheme(cinder::Dev::Panels::editorTheme());
    ui.app.setRoot(outliner.widget());
    ui.size = {320.0f, 400.0f};
    ui.frame();

    Actor& box = world.node("Box");
    world.selection.select(box.rootComponent()->id());
    ui.frame();
    CHECK(outliner.selectedActor() == box.id());
    CHECK(outliner.tree()->isSelected(static_cast<ItemId>(box.id())));
}

namespace {

bool hasLabel(Widget* widget, const std::string& text) {
    if (widget == nullptr) return false;
    if (auto* label = dynamic_cast<Label*>(widget); label != nullptr && label->text() == text) return true;
    for (int i = 0; i < widget->childCount(); ++i) {
        if (hasLabel(widget->childAt(i), text)) return true;
    }
    return false;
}

Button* buttonNamed(Widget* widget, const std::string& text) {
    if (widget == nullptr) return nullptr;
    if (auto* button = dynamic_cast<Button*>(widget); button != nullptr && hasLabel(button, text)) return button;
    for (int i = 0; i < widget->childCount(); ++i) {
        if (Button* found = buttonNamed(widget->childAt(i), text)) return found;
    }
    return nullptr;
}

}

TEST_CASE("Details lists an actor's components and a click selects one") {
    World world;
    Actor& box = world.node("Box");
    auto* extra = box.addComponent<cinder::Scene::SceneComponent>("Pivot");
    Harness ui(nullptr);
    Details details(world.selection, world.history, world.scene, ui.app);
    ui.app.setTheme(cinder::Dev::Panels::editorTheme());
    ui.app.setRoot(details.widget());
    ui.size = {320.0f, 700.0f};
    ui.frame();

    world.selection.select(box.id());
    details.update();
    ui.frame();
    CHECK(hasLabel(details.widget().get(), "Components"));
    CHECK(hasLabel(details.widget().get(), "StaticMeshComponent"));
    Button* pivot = buttonNamed(details.widget().get(), "Pivot  (SceneComponent)");
    REQUIRE(pivot != nullptr);
    CHECK(buttonNamed(details.widget().get(), "x") != nullptr);

    const Rect rect = ui.app.grid().geometryOf(pivot).value_or(Geometry{}).rect();
    ui.click(rect.center().x, rect.center().y);
    CHECK(world.selection.id() == extra->id());
    details.update();
    ui.frame();
    CHECK(hasLabel(details.widget().get(), "Pivot (SceneComponent)"));
}

TEST_CASE("Details refuses to remove a default component and removes an added one as one undo step") {
    World world;
    Actor& box = world.node("Box");
    const int extra = box.addComponent<cinder::Scene::SceneComponent>("Pivot")->id();
    world.history.reset();
    Harness ui(nullptr);
    Details details(world.selection, world.history, world.scene, ui.app);
    ui.app.setTheme(cinder::Dev::Panels::editorTheme());
    ui.app.setRoot(details.widget());
    ui.size = {320.0f, 700.0f};
    world.selection.select(box.id());
    details.update();
    ui.frame();

    Button* remove = buttonNamed(details.widget().get(), "x");
    REQUIRE(remove != nullptr);
    const Rect rect = ui.app.grid().geometryOf(remove).value_or(Geometry{}).rect();
    ui.click(rect.center().x, rect.center().y);
    CHECK(world.scene.byId(extra) == nullptr);
    CHECK(box.components().size() == 1);
    world.history.settle(false);
    REQUIRE(world.history.canUndo());

    world.history.undo(world.selection);
    CHECK(world.scene.byId(extra) != nullptr);
}

TEST_CASE("a modal dialog keeps every key from the global shortcuts") {
    int shortcuts = 0;
    int confirmed = 0;
    int cancelled = 0;
    std::shared_ptr<cinder::Dev::Panels::Dialog> dialog =
            make<cinder::Dev::Panels::Dialog>()
                    .title("Unsaved Changes")
                    .message("Save?")
                    .choices({cinder::Dev::Panels::DialogChoice{"Save", [&] { ++confirmed; }, true, {}}})
                    .onCancel([&] { ++cancelled; });
    Harness ui(dialog);
    ui.app.setTheme(cinder::Dev::Panels::editorTheme());
    auto commands = std::make_shared<CommandList>();
    auto play = std::make_shared<Command>(Command{"Play", "Play", "", Shortcut::primary(keys::letter('p'))});
    commands->map(play, CommandAction{[&] { ++shortcuts; }, {}, {}});
    ui.app.addCommands(commands);
    ui.frame();
    ui.app.setFocus(dialog);

    ui.key(keys::letter('p'), cinder::Platform::modifiers::PRIMARY);
    ui.key(keys::TAB);
    CHECK(shortcuts == 0);
    CHECK(ui.app.focused() == dialog);
    ui.key(keys::ENTER);
    CHECK(confirmed == 1);
    ui.key(keys::ESCAPE);
    CHECK(cancelled == 1);
}

namespace {

struct Aim final : cinder::Scene::Component {
    cinder::Scene::Ref<Actor> target_;
    CINDER_COMPONENT(Aim, cinder::Scene::Component) { CINDER_PROP(target_); }
};

struct AimWorld : World {
    Aim* aim = nullptr;

    AimWorld() {
        types.add<Aim>("Aim");
        aim = static_cast<Aim*>(scene.addComponent(node("Box"), "Aim"));
        history.reset();
    }
};

Menu* openMenuBelow(Harness& ui, ComboButton* combo) {
    const Rect rect = ui.app.grid().geometryOf(combo).value_or(Geometry{}).rect();
    ui.click(rect.center().x, rect.center().y);
    ui.frame();
    for (float y = rect.max.y + 1.0f; y < rect.max.y + 60.0f; y += 2.0f) {
        for (const PathEntry& entry : ui.app.grid().pathAt({rect.center().x, y})) {
            if (auto* menu = dynamic_cast<Menu*>(entry.widget.get())) return menu;
        }
    }
    return nullptr;
}

ComboButton* comboShowing(Widget* widget, const std::string& text) {
    if (widget == nullptr) return nullptr;
    if (auto* combo = dynamic_cast<ComboButton*>(widget); combo != nullptr && hasLabel(combo, text)) return combo;
    for (int i = 0; i < widget->childCount(); ++i) {
        if (ComboButton* found = comboShowing(widget->childAt(i), text)) return found;
    }
    return nullptr;
}

int rowLabelled(Menu& menu, const std::string& text) {
    for (int i = 0; i < menu.rowCount(); ++i) {
        if (menu.item(i).label.get() == text) return i;
    }
    return -1;
}

}

TEST_CASE("a reference field lists the objects it accepts and writes the choice as one undo step") {
    AimWorld world;
    Harness ui(nullptr);
    Details details(world.selection, world.history, world.scene, ui.app);
    ui.app.setTheme(cinder::Dev::Panels::editorTheme());
    ui.app.setRoot(details.widget());
    ui.size = {360.0f, 900.0f};
    world.selection.select(world.aim->id());
    details.update();
    ui.frame();

    CHECK(hasLabel(details.widget().get(), "None"));
    auto* combo = comboShowing(details.widget().get(), "None");
    REQUIRE(combo != nullptr);
    Menu* menu = openMenuBelow(ui, combo);
    REQUIRE(menu != nullptr);
    CHECK(rowLabelled(*menu, "None") >= 0);
    CHECK(rowLabelled(*menu, "Box") >= 0);
    CHECK(rowLabelled(*menu, "Flag") >= 0);
    CHECK(rowLabelled(*menu, "Box \xE2\x80\xBA Aim") < 0);

    menu->activate(rowLabelled(*menu, "Flag"));
    ui.frame();
    CHECK(world.aim->target_.get(world.scene) == &world.node("Flag"));
    CHECK(world.selection.id() == world.aim->id());
    CHECK(hasLabel(details.widget().get(), "Flag"));
    world.history.settle(false);
    REQUIRE(world.history.canUndo());

    world.history.undo(world.selection);
    auto* restored = world.node("Box").findComponent<Aim>();
    REQUIRE(restored != nullptr);
    CHECK(restored->target_.id == 0);
}

TEST_CASE("the eyedropper takes the next Outliner click without changing the selection") {
    AimWorld world;
    Harness ui(nullptr);
    Details details(world.selection, world.history, world.scene, ui.app);
    Outliner outliner(world.selection, world.history, world.scene, ui.app);
    ui.app.setTheme(cinder::Dev::Panels::editorTheme());
    ui.app.setRoot(make<HorizontalBox>()
                   + HorizontalBox::slot().fill(1.0f)[outliner.widget()]
                   + HorizontalBox::slot().fill(1.0f)[details.widget()]);
    ui.size = {720.0f, 900.0f};
    world.selection.select(world.aim->id());
    details.update();
    ui.frame();

    Button* pick = buttonNamed(details.widget().get(), "Pick");
    REQUIRE(pick != nullptr);
    Rect rect = ui.app.grid().geometryOf(pick).value_or(Geometry{}).rect();
    ui.click(rect.center().x, rect.center().y);
    CHECK(world.selection.pick().armed());

    const auto props = static_cast<ItemId>(world.node("Props").id());
    rect = ui.rectOf(outliner.tree()->rowFor(props));
    ui.click(rect.center().x, rect.center().y);
    ui.frame();
    CHECK_FALSE(world.selection.pick().armed());
    CHECK(world.aim->target_.get(world.scene) == &world.node("Props"));
    CHECK(world.selection.id() == world.aim->id());

    rect = ui.app.grid().geometryOf(pick).value_or(Geometry{}).rect();
    ui.click(rect.center().x, rect.center().y);
    CHECK(world.selection.pick().armed());
    ui.click(rect.center().x, rect.center().y);
    CHECK_FALSE(world.selection.pick().armed());

    ui.click(rect.center().x, rect.center().y);
    world.selection.select(world.node("Box").id());
    CHECK_FALSE(world.selection.pick().armed());
}

TEST_CASE("an armed pick skips what it cannot take and tries the actor's components") {
    AimWorld world;
    int taken = 0;
    world.selection.pick().arm(&taken, [&](cinder::Scene::Object& object) {
        if (dynamic_cast<Aim*>(&object) == nullptr) return false;
        taken = object.id();
        return true;
    });
    CHECK_FALSE(world.selection.pick().offer(world.node("Flag")));
    CHECK(world.selection.pick().armedBy(&taken));
    CHECK(world.selection.pick().offer(world.node("Box")));
    CHECK(taken == world.aim->id());
    CHECK_FALSE(world.selection.pick().armed());
}

TEST_CASE("the Open Scene picker filters as you type and opens the highlighted scene") {
    std::optional<std::string> opened;
    int cancelled = 0;
    std::shared_ptr<cinder::Dev::Panels::OpenSceneDialog> picker =
            make<cinder::Dev::Panels::OpenSceneDialog>()
                    .current("Scenes/main.scene")
                    .onOpen([&](const std::string& name) { opened = name; })
                    .onCancel([&] { ++cancelled; });
    Harness ui(picker);
    ui.app.setTheme(cinder::Dev::Panels::editorTheme());
    picker->show({"Scenes/Levels/cave.scene", "Scenes/Levels/forest.scene", "Scenes/main.scene"});
    ui.frame();
    ui.app.setFocus(picker->focusTarget());

    CHECK(picker->chosen() == "Scenes/main.scene");
    ui.type(U"lev");
    CHECK(picker->matches().size() == 2);
    CHECK(picker->chosen() == "Scenes/Levels/cave.scene");
    ui.key(keys::DOWN);
    ui.key(keys::DOWN);
    CHECK(picker->chosen() == "Scenes/Levels/forest.scene");
    ui.key(keys::letter('p'), cinder::Platform::modifiers::PRIMARY);
    CHECK_FALSE(opened.has_value());
    ui.key(keys::ENTER);
    CHECK(opened == "Scenes/Levels/forest.scene");

    opened.reset();
    ui.type(U"zzz");
    CHECK(picker->matches().empty());
    ui.key(keys::ENTER);
    CHECK_FALSE(opened.has_value());
    ui.key(keys::ESCAPE);
    CHECK(cancelled == 1);
}

TEST_CASE("the Save Scene As dialog resolves the name under Content and refuses one outside it") {
    const std::filesystem::path root = testing::uniqueTempDir("cinder_save_as_test");
    std::filesystem::create_directories(root / "Content" / "Scenes");
    cinder::Platform::writeTextFile(root / "Content" / "Scenes" / "main.scene", "version 5\n");
    cinder::Platform::setProjectRoot(root);

    std::optional<std::filesystem::path> saved;
    std::shared_ptr<cinder::Dev::Panels::SaveSceneDialog> dialog =
            make<cinder::Dev::Panels::SaveSceneDialog>().onSave([&](const std::filesystem::path& path) { saved = path; });
    Harness ui(dialog);
    ui.app.setTheme(cinder::Dev::Panels::editorTheme());
    dialog->show("Scenes/main.scene");
    ui.frame();
    ui.app.setFocus(dialog->focusTarget());

    CHECK(dialog->resolved().exists);
    ui.type(U"../outside");
    CHECK_FALSE(dialog->resolved().error.empty());
    ui.key(keys::ENTER);
    CHECK_FALSE(saved.has_value());

    dialog->show("Scenes/level");
    ui.key(keys::ENTER);
    REQUIRE(saved.has_value());
    CHECK(*saved == root / "Content" / "Scenes" / "level.scene");
    CHECK_FALSE(dialog->resolved().exists);
    std::filesystem::remove_all(root);
}

namespace {

struct Instance : World {
    int root = 0;
    int part = 0;
    int pivot = 0;

    Instance() {
        Actor* crate = scene.spawn("StaticMeshActor");
        crate->setName("Crate");
        crate->setPrefab("Prefabs/crate.prefab");
        crate->setPrefabId(1);
        crate->rootComponent()->setPrefabId(2);
        pivot = crate->addComponent<cinder::Scene::SceneComponent>("Pivot")->id();
        scene.component(pivot)->setPrefabId(3);
        Actor* lid = scene.spawn("Actor", crate);
        lid->setName("Lid");
        lid->setPrefabId(4);
        root = crate->id();
        part = lid->id();
        history.reset();
    }
};

}

TEST_CASE("the Outliner keeps a part of a prefab instance from being deleted") {
    Instance world;
    Harness ui(nullptr);
    Outliner outliner(world.selection, world.history, world.scene, ui.app);
    ui.app.setTheme(cinder::Dev::Panels::editorTheme());
    ui.app.setRoot(outliner.widget());
    ui.size = {320.0f, 400.0f};
    world.selection.select(world.part, true);
    outliner.update();
    ui.frame();

    const Rect row = ui.rectOf(outliner.tree()->rowFor(static_cast<ItemId>(world.part)));
    ui.click(row.center().x, row.center().y);
    ui.key(keys::DELETE_KEY);
    ui.frame();
    CHECK(world.scene.actor(world.part) != nullptr);
}

TEST_CASE("Details shows a prefab instance's source, hides remove on what it inherits, and runs its actions") {
    Instance world;
    Harness ui(nullptr);
    Details details(world.selection, world.history, world.scene, ui.app);
    std::optional<int> reverted;
    cinder::Dev::PrefabActions actions;
    actions.revert = [&](int id) { reverted = id; };
    actions.enabled = [] { return true; };
    details.setPrefabActions(actions);
    ui.app.setTheme(cinder::Dev::Panels::editorTheme());
    ui.app.setRoot(details.widget());
    ui.size = {360.0f, 900.0f};
    world.selection.select(world.part);
    details.update();
    ui.frame();

    Button* revert = buttonNamed(details.widget().get(), "Revert");
    REQUIRE(revert != nullptr);
    const Rect rect = ui.app.grid().geometryOf(revert).value_or(Geometry{}).rect();
    ui.click(rect.center().x, rect.center().y);
    CHECK(reverted == world.root);

    world.selection.select(world.root);
    details.update();
    ui.frame();
    CHECK(buttonNamed(details.widget().get(), "x") == nullptr);
}
