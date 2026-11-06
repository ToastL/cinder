#include <doctest/doctest.h>

#include "components/Builtins.hpp"
#include "dev/History.hpp"
#include "dev/Selection.hpp"
#include "scene/Node.hpp"
#include "scene/NodeTypes.hpp"
#include "scene/Scene.hpp"
#include "scene/Transform.hpp"
#include "serial/SceneCodec.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

using cinder::dev::History;
using cinder::dev::Selection;
using cinder::scene::Node;
using cinder::scene::NodeTypes;
using cinder::scene::Scene;

namespace {

struct Editor {
    NodeTypes types;
    Scene scene{types};
    Selection selection;
    History history{scene};

    Editor() {
        cinder::components::registerBuiltins(types);
        scene.create("MeshPart", nullptr)->setName("Box");
        history.reset();
    }

    Node& box() { return *scene.find("Box"); }
    float x() { return box().transform()->position().x; }

    void move(float to, bool interacting = false) {
        box().transform()->setPosition(to, 0.0f, 0.0f);
        history.touch("Move", selection.id());
        history.settle(interacting);
    }
};

}

TEST_CASE("an edit undoes and redoes") {
    Editor editor;
    editor.move(3.0f);
    REQUIRE(editor.history.canUndo());
    CHECK(editor.history.undoLabel() == "Move");

    editor.history.undo(editor.selection);
    CHECK(editor.x() == doctest::Approx(0.0f));

    editor.history.redo(editor.selection);
    CHECK(editor.x() == doctest::Approx(3.0f));
}

TEST_CASE("edits made while interacting are one step") {
    Editor editor;
    editor.move(1.0f, true);
    editor.move(2.0f, true);
    editor.move(3.0f, true);
    CHECK_FALSE(editor.history.canUndo());

    editor.history.settle(false);
    CHECK(editor.history.steps() == 1);

    editor.history.undo(editor.selection);
    CHECK(editor.x() == doctest::Approx(0.0f));
}

TEST_CASE("an edit that changes nothing records nothing") {
    Editor editor;
    editor.move(0.0f);
    CHECK_FALSE(editor.history.canUndo());
    CHECK_FALSE(editor.history.dirty());
}

TEST_CASE("a new edit clears redo") {
    Editor editor;
    editor.move(1.0f);
    editor.history.undo(editor.selection);
    CHECK(editor.history.canRedo());

    editor.move(2.0f);
    CHECK_FALSE(editor.history.canRedo());
}

TEST_CASE("undoing a delete restores the node, its id, its place and the selection") {
    Editor editor;
    editor.scene.create("Group", nullptr)->setName("Last");
    editor.history.reset();

    const int id = editor.box().id();
    editor.selection.select(id);
    editor.history.touch("Delete", editor.selection.id());
    editor.scene.destroyNow(&editor.box());
    editor.selection.clear();
    editor.history.settle(false);
    REQUIRE(editor.scene.roots().size() == 1);

    editor.history.undo(editor.selection);
    REQUIRE(editor.scene.roots().size() == 2);
    CHECK(editor.scene.roots()[0]->name() == "Box");
    CHECK(editor.scene.roots()[0]->id() == id);
    CHECK(editor.scene.roots()[1]->name() == "Last");
    CHECK(editor.selection.id() == id);
}

TEST_CASE("undo and redo restore the selection on each side of an insert") {
    Editor editor;
    editor.history.touch("Insert", editor.selection.id());
    const int inserted = editor.scene.create("Group", nullptr)->id();
    editor.selection.select(inserted);
    editor.history.settle(false);

    editor.history.undo(editor.selection);
    CHECK(editor.scene.byId(inserted) == nullptr);
    CHECK_FALSE(editor.selection.id().has_value());

    editor.history.redo(editor.selection);
    CHECK(editor.scene.byId(inserted) != nullptr);
    CHECK(editor.selection.id() == inserted);
}

TEST_CASE("the scene is dirty until saved, and clean again when undone to the saved state") {
    Editor editor;
    editor.move(1.0f);
    CHECK(editor.history.dirty());

    editor.history.undo(editor.selection);
    CHECK_FALSE(editor.history.dirty());

    editor.history.redo(editor.selection);
    CHECK(editor.history.dirty());

    const std::filesystem::path path =
            std::filesystem::temp_directory_path() / "cinder_history_test" / "main.scene";
    editor.history.save(path);
    CHECK_FALSE(editor.history.dirty());

    std::ifstream in(path, std::ios::binary);
    std::ostringstream text;
    text << in.rdbuf();
    CHECK(text.str() == cinder::serial::SceneCodec::save(editor.scene));
    std::filesystem::remove_all(path.parent_path());
}

TEST_CASE("edits made while history is disabled are not recorded") {
    Editor editor;
    editor.history.setEnabled(false);
    editor.move(5.0f);
    editor.history.setEnabled(true);
    editor.history.settle(false);
    CHECK_FALSE(editor.history.canUndo());
}

TEST_CASE("history keeps at most MAX_STEPS") {
    Editor editor;
    for (std::size_t i = 0; i < History::MAX_STEPS + 5; ++i) editor.move(static_cast<float>(i + 1));
    CHECK(editor.history.steps() == History::MAX_STEPS);
}
