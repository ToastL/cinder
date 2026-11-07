#pragma once

#include "UI/Core/Args.hpp"
#include "UI/Core/CompoundWidget.hpp"
#include "UI/Core/Delegates.hpp"
#include "UI/Core/TextRun.hpp"
#include "UI/Framework/Application.hpp"
#include "UI/Framework/Commands.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace cinder::UI {

class MenuBuilder;
class Menu;

struct MenuItem {
    enum class Kind : std::uint8_t { Entry, SubMenu, Separator, Heading, Custom };

    Kind kind = Kind::Entry;
    Attribute<std::string> label;
    std::string shortcut;
    std::string toolTip;
    std::function<void()> action;
    std::function<bool()> canExecute;
    std::function<bool()> isChecked;
    std::function<void(MenuBuilder&)> subMenu;
    std::shared_ptr<Widget> widget;
    bool checkable = false;
};

class MenuBuilder {
public:
    MenuBuilder& entry(Attribute<std::string> label, std::function<void()> action, std::string shortcut = {});
    MenuBuilder& check(Attribute<std::string> label, std::function<void()> action, std::function<bool()> checked,
                       std::string shortcut = {});
    MenuBuilder& command(std::shared_ptr<const CommandList> commands, std::shared_ptr<const Command> command);
    MenuBuilder& subMenu(Attribute<std::string> label, std::function<void(MenuBuilder&)> fill);
    MenuBuilder& separator();
    MenuBuilder& heading(std::string text);
    MenuBuilder& widget(std::shared_ptr<Widget> content);
    MenuBuilder& enabledIf(std::function<bool()> canExecute);
    MenuBuilder& toolTip(std::string text);
    MenuBuilder& add(MenuItem item);

    bool empty() const { return items_.empty(); }
    const std::vector<MenuItem>& items() const { return items_; }
    std::shared_ptr<Menu> build(std::string style = "Menu") const;

private:
    std::vector<MenuItem> items_;
};

class MenuRow : public LeafWidget {
public:
    MenuRow(Menu& menu, int index) : menu_(&menu), index_(index) {}

    int index() const { return index_; }
    const Rect& paintedRect() const { return painted_; }

    void onMouseEnter(const Geometry& geometry, const PointerEvent& event) override;
    Reply onMouseDown(const Geometry& geometry, const PointerEvent& event) override;
    Reply onMouseUp(const Geometry& geometry, const PointerEvent& event) override;

protected:
    glm::vec2 computeDesiredSize(float layoutScale) const override;
    int onPaint(const PaintArgs& args, const Geometry& geometry, ElementList& list, int layer,
                const PaintStyle& style, bool enabled) const override;

private:
    Menu* menu_;
    int index_;
    mutable Rect painted_;
    mutable TextRun label_;
    mutable TextRun shortcut_;
};

class Menu : public CompoundWidget {
public:
    static constexpr double SUBMENU_DELAY = 0.25;

    struct Args : ::cinder::UI::Args<Args, Menu> {
        UI_ARG(std::vector<MenuItem>, items)
        UI_ARG(std::string, style, "Menu")
    };

    void construct(const Args& args);

    const MenuItem& item(int row) const { return items_[static_cast<std::size_t>(rowItems_[static_cast<std::size_t>(row)])]; }
    const std::string& style() const { return style_; }
    int rowCount() const { return static_cast<int>(rows_.size()); }
    const std::shared_ptr<MenuRow>& row(int index) const { return rows_[static_cast<std::size_t>(index)]; }
    int highlighted() const { return highlighted_; }
    bool rowEnabled(int row) const;
    void highlight(int row, bool fromMouse);
    void activate(int row);
    void openSubMenu(int row, bool focus);
    void closeSubMenu();
    const std::shared_ptr<Menu>& subMenu() const { return sub_; }
    int subMenuRow() const { return subRow_; }
    bool isNested() const { return nested_; }

    bool supportsKeyboardFocus() const override { return true; }
    Reply onKeyDown(const Geometry& geometry, const KeyEvent& event) override;
    Reply onMouseDown(const Geometry& geometry, const PointerEvent& event) override;
    void tick(const Geometry& geometry, double time, float deltaTime) override;

protected:
    glm::vec2 computeDesiredSize(float layoutScale) const override;
    void arrangeChildren(const Geometry& geometry, ArrangedChildren& out) const override;
    int onPaint(const PaintArgs& args, const Geometry& geometry, ElementList& list, int layer,
                const PaintStyle& style, bool enabled) const override;

private:
    int step(int from, int direction) const;

    std::vector<MenuItem> items_;
    std::vector<int> rowItems_;
    std::vector<std::shared_ptr<MenuRow>> rows_;
    std::string style_;
    int highlighted_ = -1;
    double highlightedAt_ = 0.0;
    bool fromMouse_ = false;
    int subRow_ = -1;
    std::shared_ptr<Menu> sub_;
    bool nested_ = false;
};

void showContextMenu(std::shared_ptr<Widget> menu, glm::vec2 at);

}
