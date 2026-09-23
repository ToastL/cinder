#pragma once

#include "ui/core/Args.hpp"
#include "ui/core/CompoundWidget.hpp"
#include "ui/core/TextRun.hpp"
#include "ui/docking/TabManager.hpp"

#include <optional>
#include <string>
#include <vector>

namespace cinder::ui {

class TabManager;
class TabStack;

class TabDragDrop : public DragDropOperation {
public:
    TabDragDrop(std::string tab, std::string label);

    const std::string& tab() const { return tab_; }
    std::shared_ptr<Widget> decorator() const override { return decorator_; }

private:
    std::string tab_;
    std::shared_ptr<Widget> decorator_;
};

class DockTab : public LeafWidget {
public:
    DockTab(TabManager& manager, TabStack& stack, std::string tab);

    const std::string& tab() const { return tab_; }

    Reply onMouseDown(const Geometry& geometry, const PointerEvent& event) override;
    Reply onMouseUp(const Geometry& geometry, const PointerEvent& event) override;
    Reply onDragDetected(const Geometry& geometry, const PointerEvent& event) override;

protected:
    glm::vec2 computeDesiredSize(float layoutScale) const override;
    int onPaint(const PaintArgs& args, const Geometry& geometry, ElementList& list, int layer,
                const PaintStyle& style, bool enabled) const override;

private:
    Rect closeRect(const Geometry& geometry) const;
    bool closable() const;

    TabManager* manager_;
    TabStack* stack_;
    std::string tab_;
    mutable TextRun label_;
};

class TabStack : public CompoundWidget {
public:
    TabStack(TabManager& manager, int id, std::vector<std::string> tabs, std::string active);

    int id() const { return id_; }
    const std::string& active() const { return active_; }
    const std::vector<std::string>& tabs() const { return tabs_; }
    std::shared_ptr<DockTab> tabWidget(const std::string& tab) const;
    void activate(const std::string& tab);
    std::optional<DockSide> hoveredSide() const { return side_; }
    void build();

    void onDragEnter(const Geometry& geometry, const DragDropEvent& event) override;
    void onDragLeave(const DragDropEvent& event) override;
    Reply onDragOver(const Geometry& geometry, const DragDropEvent& event) override;
    Reply onDrop(const Geometry& geometry, const DragDropEvent& event) override;

protected:
    int onPaint(const PaintArgs& args, const Geometry& geometry, ElementList& list, int layer,
                const PaintStyle& style, bool enabled) const override;

private:
    std::optional<DockSide> sideAt(const Geometry& geometry, glm::vec2 point, const DragDropEvent& event) const;

    TabManager* manager_;
    int id_;
    std::vector<std::string> tabs_;
    std::vector<std::shared_ptr<DockTab>> widgets_;
    std::shared_ptr<CompoundWidget> content_;
    std::string active_;
    std::optional<DockSide> side_;
};

}
