#include <doctest/doctest.h>

#include "ui_harness.hpp"

#include "ui/docking/DockTab.hpp"
#include "ui/docking/TabManager.hpp"
#include "ui/widgets/Menu.hpp"

#include <string>
#include <vector>

using namespace cinder::ui;
using uitest::Harness;
using uitest::Probe;

namespace {

struct Editor {
    TabManager manager;
    std::shared_ptr<Widget> area;

    Editor() {
        manager.registerTab("Scene", "Scene", [] { return make<Probe>(); }, false);
        manager.registerTab("Console", "Console", [] { return make<Probe>(); });
        manager.registerTab("Explorer", "Explorer", [] { return make<Probe>(); });
        area = manager.restore(TabManager::split(Orientation::Vertical,
                                                 {TabManager::stack({"Scene"}, 3.0f),
                                                  TabManager::stack({"Console", "Explorer"}, 1.0f)}));
    }

    int stackOf(const std::string& tab) const {
        for (int id = 1; id < 64; ++id) {
            const std::vector<std::string> tabs = manager.tabsIn(id);
            if (std::find(tabs.begin(), tabs.end(), tab) != tabs.end()) return id;
        }
        return -1;
    }

    std::shared_ptr<TabStack> stack(const std::string& tab) const { return manager.stackWidget(stackOf(tab)); }
};

float closeX(const Rect& tab) { return tab.max.x - 19.0f; }

void dragTab(Harness& ui, const Rect& from, const Rect& onto, glm::vec2 offset) {
    ui.move(from.center().x, from.center().y);
    ui.press();
    ui.move(from.center().x + 10.0f, from.center().y + 6.0f);
    ui.frame();
    ui.move(onto.min.x + offset.x, onto.min.y + offset.y);
}

}

TEST_CASE("a restored layout stacks its tabs, and a click on a tab shows its panel") {
    Editor editor;
    Harness ui(editor.area);
    REQUIRE(editor.manager.stackCount() == 2);
    CHECK(editor.manager.isOpen("Scene"));
    CHECK(editor.manager.isActive("Console"));
    CHECK_FALSE(editor.manager.isActive("Explorer"));

    const std::shared_ptr<TabStack> bottom = editor.stack("Console");
    REQUIRE(bottom);
    CHECK(bottom->tabs() == std::vector<std::string>{"Console", "Explorer"});
    const Rect explorer = ui.rectOf(bottom->tabWidget("Explorer"));
    ui.click(explorer.center().x, explorer.center().y);
    ui.frame();
    CHECK(editor.manager.isActive("Explorer"));
    CHECK(bottom->active() == "Explorer");

    const Rect scene = ui.rectOf(editor.stack("Scene"));
    CHECK(scene.height() > ui.rectOf(bottom).height());
}

TEST_CASE("closing the last tab of a stack collapses it, and the Window menu brings the tab back") {
    Editor editor;
    Harness ui(editor.area);
    const std::shared_ptr<TabStack> bottom = editor.stack("Console");
    const Rect console = ui.rectOf(bottom->tabWidget("Console"));
    ui.click(closeX(console), console.center().y);
    ui.frame();
    CHECK_FALSE(editor.manager.isOpen("Console"));
    CHECK(editor.manager.stackCount() == 2);

    const std::shared_ptr<TabStack> left = editor.stack("Explorer");
    REQUIRE(left);
    const Rect explorer = ui.rectOf(left->tabWidget("Explorer"));
    ui.platform.advance(1.0);
    ui.click(closeX(explorer), explorer.center().y);
    ui.frame();
    CHECK_FALSE(editor.manager.isOpen("Explorer"));
    CHECK(editor.manager.stackCount() == 1);
    CHECK(editor.manager.layout().kind == LayoutNode::Kind::Stack);

    MenuBuilder menu;
    editor.manager.fillWindowMenu(menu);
    REQUIRE(menu.items().size() == 3);
    CHECK(menu.items()[0].isChecked());
    CHECK_FALSE(menu.items()[0].canExecute());
    CHECK_FALSE(menu.items()[1].isChecked());
    menu.items()[1].action();
    ui.frame();
    CHECK(editor.manager.isOpen("Console"));
    CHECK(editor.manager.tabsIn(editor.stackOf("Console")) == std::vector<std::string>{"Scene", "Console"});
}

TEST_CASE("a tab dropped in the middle of another stack joins it") {
    Editor editor;
    Harness ui(editor.area);
    const std::shared_ptr<TabStack> bottom = editor.stack("Console");
    const std::shared_ptr<TabStack> top = editor.stack("Scene");
    const Rect explorer = ui.rectOf(bottom->tabWidget("Explorer"));
    const Rect scene = ui.rectOf(top);

    dragTab(ui, explorer, scene, scene.size() * 0.5f);
    CHECK(top->hoveredSide() == std::optional<DockSide>(DockSide::Centre));
    ui.release();
    ui.frame();
    CHECK(editor.manager.stackCount() == 2);
    CHECK(editor.manager.tabsIn(editor.stackOf("Scene")) == std::vector<std::string>{"Scene", "Explorer"});
    CHECK(editor.manager.tabsIn(editor.stackOf("Console")) == std::vector<std::string>{"Console"});
    CHECK(editor.manager.isActive("Explorer"));
    CHECK(editor.manager.layout().kind == LayoutNode::Kind::Split);
}

TEST_CASE("a tab dropped against an edge splits the stack it lands on") {
    Editor editor;
    Harness ui(editor.area);
    const std::shared_ptr<TabStack> bottom = editor.stack("Console");
    const std::shared_ptr<TabStack> top = editor.stack("Scene");
    const Rect explorer = ui.rectOf(bottom->tabWidget("Explorer"));
    const Rect scene = ui.rectOf(top);

    dragTab(ui, explorer, scene, {8.0f, scene.height() * 0.5f});
    CHECK(top->hoveredSide() == std::optional<DockSide>(DockSide::Left));
    ui.release();
    ui.frame();
    CHECK(editor.manager.stackCount() == 3);
    const LayoutNode& root = editor.manager.layout();
    REQUIRE(root.kind == LayoutNode::Kind::Split);
    REQUIRE(root.children.size() == 2);
    const LayoutNode& first = root.children[0];
    REQUIRE(first.kind == LayoutNode::Kind::Split);
    CHECK(first.orientation == Orientation::Horizontal);
    CHECK(first.children[0].tabs == std::vector<std::string>{"Explorer"});
    CHECK(first.children[1].tabs == std::vector<std::string>{"Scene"});
}

TEST_CASE("the only tab of a stack cannot split that stack, and a cancelled drag changes nothing") {
    Editor editor;
    Harness ui(editor.area);
    const std::shared_ptr<TabStack> top = editor.stack("Scene");
    const Rect sceneTab = ui.rectOf(top->tabWidget("Scene"));
    const Rect scene = ui.rectOf(top);

    dragTab(ui, sceneTab, scene, {8.0f, scene.height() * 0.5f});
    ui.release();
    ui.frame();
    CHECK(editor.manager.stackCount() == 2);
    CHECK(editor.manager.tabsIn(editor.stackOf("Scene")) == std::vector<std::string>{"Scene"});

    const std::shared_ptr<TabStack> bottom = editor.stack("Console");
    const Rect explorer = ui.rectOf(bottom->tabWidget("Explorer"));
    ui.platform.advance(1.0);
    dragTab(ui, explorer, scene, scene.size() * 0.5f);
    ui.key(cinder::platform::keys::ESCAPE);
    ui.release();
    ui.frame();
    CHECK(editor.manager.stackCount() == 2);
    CHECK(editor.manager.tabsIn(editor.stackOf("Explorer")) == std::vector<std::string>{"Console", "Explorer"});
}
