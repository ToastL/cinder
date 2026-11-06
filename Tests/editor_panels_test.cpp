#include <doctest/doctest.h>

#include "ui_harness.hpp"

#include "components/Builtins.hpp"
#include "dev/History.hpp"
#include "dev/Selection.hpp"
#include "dev/panels/EditorStyle.hpp"
#include "dev/panels/Explorer.hpp"
#include "dev/panels/Properties.hpp"
#include "scene/Node.hpp"
#include "scene/NodeTypes.hpp"
#include "scene/PropValue.hpp"
#include "scene/Scene.hpp"
#include "scene/Transform.hpp"
#include "ui/widgets/Label.hpp"
#include "ui/widgets/SpinBox.hpp"
#include "ui/widgets/TextField.hpp"
#include "ui/widgets/TreeView.hpp"

#include <string>
#include <vector>

using namespace cinder::ui;
using cinder::dev::History;
using cinder::dev::Selection;
using cinder::dev::panels::Explorer;
using cinder::dev::panels::Properties;
using cinder::scene::Node;
using cinder::scene::NodeTypes;
using cinder::scene::PropValue;
using cinder::scene::Scene;
using uitest::Harness;
namespace keys = cinder::platform::keys;

namespace {

struct World {
    NodeTypes types;
    Scene scene{types};
    Selection selection;
    History history{scene};

    World() {
        cinder::components::registerBuiltins(types);
        Node* box = scene.create("MeshPart", nullptr);
        box->setName("Box");
        box->setAttribute("phase", PropValue::number(2.0));
        Node* group = scene.create("Group", nullptr);
        group->setName("Props");
        scene.create("Sprite", group)->setName("Flag");
        history.reset();
    }

    Node& node(const std::string& name) { return *scene.find(name); }
};

}

TEST_CASE("the Explorer lists the tree, selects a row and deletes the selection with a key") {
    World world;
    Harness ui(nullptr);
    Explorer explorer(world.selection, world.history, world.scene, ui.app);
    ui.app.setTheme(cinder::dev::panels::editorTheme());
    ui.app.setRoot(explorer.widget());
    ui.size = {320.0f, 400.0f};
    ui.frame();

    const std::shared_ptr<TreeView> tree = explorer.tree();
    REQUIRE(tree->visibleCount() == 2);

    const auto box = static_cast<ItemId>(world.node("Box").id());
    const Rect row = ui.rectOf(tree->rowFor(box));
    ui.click(row.center().x, row.center().y);
    CHECK(world.selection.id() == world.node("Box").id());
    CHECK(tree->isSelected(box));
    CHECK(ui.app.focused() == tree);

    ui.key(keys::DELETE);
    ui.frame();
    CHECK(world.scene.find("Box") == nullptr);
    CHECK_FALSE(world.selection.id().has_value());
    CHECK(tree->visibleCount() == 1);
    world.history.settle(false);
    CHECK(world.history.canUndo());
}

TEST_CASE("the Explorer's filter flattens the tree to what matches") {
    World world;
    Harness ui(nullptr);
    Explorer explorer(world.selection, world.history, world.scene, ui.app);
    ui.app.setTheme(cinder::dev::panels::editorTheme());
    ui.app.setRoot(explorer.widget());
    ui.size = {320.0f, 400.0f};
    ui.frame();

    const std::shared_ptr<TreeView> tree = explorer.tree();
    CHECK(tree->visibleCount() == 2);
    CHECK(tree->hasChildren(static_cast<ItemId>(world.node("Props").id())));

    ui.click(200.0f, 14.0f);
    ui.type(U"fla");
    ui.frame();
    CHECK(tree->visibleCount() == 1);
    CHECK(tree->itemAt(0) == static_cast<ItemId>(world.node("Flag").id()));
    CHECK_FALSE(tree->hasChildren(static_cast<ItemId>(world.node("Props").id())));
}

TEST_CASE("dragging a row in the Explorer reparents the node, and refuses a cycle") {
    World world;
    Harness ui(nullptr);
    Explorer explorer(world.selection, world.history, world.scene, ui.app);
    ui.app.setTheme(cinder::dev::panels::editorTheme());
    ui.app.setRoot(explorer.widget());
    ui.size = {320.0f, 400.0f};
    ui.frame();

    const std::shared_ptr<TreeView> tree = explorer.tree();
    const Rect box = ui.rectOf(tree->rowFor(static_cast<ItemId>(world.node("Box").id())));
    const Rect props = ui.rectOf(tree->rowFor(static_cast<ItemId>(world.node("Props").id())));
    ui.move(box.center().x, box.center().y);
    ui.press();
    ui.move(box.center().x + 12.0f, box.center().y + 4.0f);
    REQUIRE(ui.app.dragDrop());
    ui.move(props.center().x, props.center().y);
    ui.release();
    ui.frame();
    CHECK(world.node("Box").parent() == &world.node("Props"));
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
    CHECK(world.node("Props").parent() == nullptr);
}

TEST_CASE("Properties edits a reflected prop through its definition and marks the scene dirty") {
    World world;
    Harness ui(nullptr);
    Properties properties(world.selection, world.history, world.scene, ui.app);
    ui.app.setTheme(cinder::dev::panels::editorTheme());
    ui.app.setRoot(properties.widget());
    ui.size = {320.0f, 500.0f};
    ui.frame();

    world.selection.select(world.node("Box").id());
    properties.update();
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
    walk(properties.widget());
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

TEST_CASE("Properties lists attributes, and rebuilds when one is added or removed") {
    World world;
    Harness ui(nullptr);
    Properties properties(world.selection, world.history, world.scene, ui.app);
    ui.app.setTheme(cinder::dev::panels::editorTheme());
    ui.app.setRoot(properties.widget());
    ui.size = {320.0f, 500.0f};
    ui.frame();

    world.selection.select(world.node("Box").id());
    properties.update();
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
        walk(properties.widget().get());
        return found;
    };
    CHECK(labelled("phase"));
    CHECK_FALSE(labelled("speed"));

    world.node("Box").setAttribute("speed", PropValue::number(4.0));
    properties.update();
    ui.frame();
    CHECK(labelled("speed"));

    world.node("Box").removeAttribute("phase");
    properties.update();
    ui.frame();
    CHECK_FALSE(labelled("phase"));

    world.selection.clear();
    properties.update();
    ui.frame();
    CHECK(labelled("Nothing selected"));
}
