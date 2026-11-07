#pragma once

#include "ui/core/Args.hpp"
#include "ui/core/CompoundWidget.hpp"
#include "ui/core/TextRun.hpp"
#include "ui/framework/PopupHandle.hpp"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace cinder::ui {

class MenuBuilder;

class MenuBar;

class MenuBarItem : public LeafWidget {
public:
    MenuBarItem(MenuBar& bar, int index) : bar_(&bar), index_(index) {}

    void onMouseEnter(const Geometry& geometry, const PointerEvent& event) override;
    Reply onMouseDown(const Geometry& geometry, const PointerEvent& event) override;

protected:
    glm::vec2 computeDesiredSize(float layoutScale) const override;
    int onPaint(const PaintArgs& args, const Geometry& geometry, ElementList& list, int layer,
                const PaintStyle& style, bool enabled) const override;

private:
    MenuBar* bar_;
    int index_;
    mutable TextRun title_;
};

struct MenuBarEntry {
    std::string title;
    std::function<void(MenuBuilder&)> fill;
};

class MenuBar : public CompoundWidget {
public:
    struct Args : ::cinder::ui::Args<Args, MenuBar> {
        std::vector<MenuBarEntry> menus_;
        Args& menu(std::string title, std::function<void(MenuBuilder&)> fill) {
            menus_.push_back(MenuBarEntry{std::move(title), std::move(fill)});
            return *this;
        }
        UI_ARG(std::string, style, "MenuBar")
    };

    void construct(const Args& args);

    const std::string& style() const { return style_; }
    const std::string& title(int index) const { return menus_[static_cast<std::size_t>(index)].title; }
    int menuCount() const { return static_cast<int>(menus_.size()); }
    int openIndex() const;
    void open(int index);
    void close();
    const std::shared_ptr<MenuBarItem>& item(int index) const { return items_[static_cast<std::size_t>(index)]; }

    Reply onMouseDown(const Geometry& geometry, const PointerEvent& event) override;

protected:
    int onPaint(const PaintArgs& args, const Geometry& geometry, ElementList& list, int layer,
                const PaintStyle& style, bool enabled) const override;

private:
    std::vector<MenuBarEntry> menus_;
    std::vector<std::shared_ptr<MenuBarItem>> items_;
    std::string style_;
    int open_ = -1;
    PopupHandle popup_;
};

}
