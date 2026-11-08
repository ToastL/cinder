#pragma once

#include "UI/Core/Args.hpp"
#include "UI/Core/Delegates.hpp"
#include "UI/Core/Widget.hpp"
#include "UI/Widgets/TreeView.hpp"

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace cinder::UI {

class ScrollBar;
class TileView;

class Tile : public Widget {
public:
    Tile(TileView& view, ItemId item, std::shared_ptr<Widget> content);

    ItemId item() const { return item_; }

    int childCount() const override { return content_ ? 1 : 0; }
    Widget* childAt(int index) const override { return index == 0 ? content_.get() : nullptr; }
    void arrangeChildren(const Geometry& geometry, ArrangedChildren& out) const override;

    Reply onMouseDown(const Geometry& geometry, const PointerEvent& event) override;
    Reply onMouseUp(const Geometry& geometry, const PointerEvent& event) override;
    Reply onMouseDoubleClick(const Geometry& geometry, const PointerEvent& event) override;
    Reply onDragDetected(const Geometry& geometry, const PointerEvent& event) override;

protected:
    glm::vec2 computeDesiredSize(float layoutScale) const override;
    int onPaint(const PaintArgs& args, const Geometry& geometry, ElementList& list, int layer,
                const PaintStyle& style, bool enabled) const override;

private:
    TileView* view_;
    ItemId item_;
    std::shared_ptr<Widget> content_;
};

class TileView : public Widget {
public:
    using Items = std::function<std::vector<ItemId>()>;
    using GenerateTile = std::function<std::shared_ptr<Widget>(ItemId)>;
    using OnSelectionChanged = std::function<void(std::optional<ItemId>, SelectInfo)>;
    using OnItem = std::function<void(ItemId)>;
    using ContextMenu = std::function<std::shared_ptr<Widget>(std::optional<ItemId>)>;
    using DragFrom = std::function<std::shared_ptr<DragDropOperation>(ItemId)>;
    using CanAcceptDrop = std::function<bool(const DragDropEvent&, std::optional<ItemId>)>;
    using AcceptDrop = std::function<void(const DragDropEvent&, std::optional<ItemId>)>;
    using KeyHandler = std::function<bool(const KeyEvent&)>;

    static constexpr float WHEEL_STEP = 0.5f;

    struct Args : ::cinder::UI::Args<Args, TileView> {
        UI_EVENT(Items, itemsSource)
        UI_EVENT(GenerateTile, onGenerateTile)
        UI_EVENT(OnSelectionChanged, onSelectionChanged)
        UI_EVENT(OnItem, onMouseButtonDoubleClick)
        UI_EVENT(ContextMenu, onContextMenuOpening)
        UI_EVENT(DragFrom, onDragDetected)
        UI_EVENT(CanAcceptDrop, onCanAcceptDrop)
        UI_EVENT(AcceptDrop, onAcceptDrop)
        UI_EVENT(KeyHandler, onKeyDownHandler)
        UI_EVENT(std::function<bool(ItemId)>, isItemSelected)
        UI_ARG(SelectionMode, selectionMode, SelectionMode::Single)
        UI_ARG(glm::vec2, tileSize, glm::vec2(88.0f, 104.0f))
        UI_ARG(float, spacing, 4.0f)
        UI_ARG(std::string, style, "TableView")
    };

    void construct(const Args& args);

    bool isSelected(ItemId item) const;
    std::vector<ItemId> selectedItems() const { return selected_; }
    void setSelection(std::optional<ItemId> item, SelectInfo how = SelectInfo::Direct);
    void clearSelection(SelectInfo how = SelectInfo::Direct);
    void requestScrollIntoView(ItemId item) { pending_ = item; }
    void refresh() { tiles_.clear(); }

    const std::string& style() const { return style_; }
    glm::vec2 tileSize() const { return tileSize_; }
    std::size_t columns() const { return columns_; }
    std::size_t itemCount() const { return items_.size(); }
    std::size_t realizedCount() const { return realized_.size(); }
    std::optional<ItemId> itemAt(std::size_t index) const;
    const std::shared_ptr<Tile>& tileFor(ItemId item) const;
    bool dropTargetIs(std::optional<ItemId> item) const { return drop_.valid && drop_.item == item; }

    void clickTile(ItemId item);
    void doubleClickTile(ItemId item);
    void openContextMenu(std::optional<ItemId> item, glm::vec2 at);
    std::shared_ptr<DragDropOperation> dragFrom(ItemId item);

    int childCount() const override;
    Widget* childAt(int index) const override;
    void arrangeChildren(const Geometry& geometry, ArrangedChildren& out) const override;
    void tick(const Geometry& geometry, double time, float deltaTime) override;

    bool supportsKeyboardFocus() const override { return true; }
    Reply onMouseDown(const Geometry& geometry, const PointerEvent& event) override;
    Reply onMouseUp(const Geometry& geometry, const PointerEvent& event) override;
    Reply onMouseWheel(const Geometry& geometry, const PointerEvent& event) override;
    Reply onKeyDown(const Geometry& geometry, const KeyEvent& event) override;
    void onDragLeave(const DragDropEvent& event) override;
    Reply onDragOver(const Geometry& geometry, const DragDropEvent& event) override;
    Reply onDrop(const Geometry& geometry, const DragDropEvent& event) override;

protected:
    glm::vec2 computeDesiredSize(float layoutScale) const override;
    int onPaint(const PaintArgs& args, const Geometry& geometry, ElementList& list, int layer,
                const PaintStyle& style, bool enabled) const override;

private:
    struct DropTarget {
        std::optional<ItemId> item;
        bool valid = false;
    };

    float pitchX() const { return tileSize_.x + spacing_; }
    float pitchY() const { return tileSize_.y + spacing_; }
    std::size_t rowCount() const;
    float contentHeight() const;
    std::size_t columnsFor(float width) const;
    void realize(const Geometry& geometry);
    std::size_t indexOf(ItemId item) const;
    std::size_t currentIndex() const;
    std::optional<ItemId> itemAtPoint(const Geometry& geometry, glm::vec2 point) const;
    void moveSelection(std::ptrdiff_t delta);

    Items source_;
    GenerateTile generate_;
    OnSelectionChanged onSelection_;
    OnItem onDoubleClick_;
    ContextMenu onContextMenu_;
    DragFrom onDrag_;
    CanAcceptDrop canDrop_;
    AcceptDrop acceptDrop_;
    KeyHandler onKey_;
    std::function<bool(ItemId)> selectedGetter_;
    SelectionMode mode_ = SelectionMode::Single;
    glm::vec2 tileSize_{88.0f, 104.0f};
    float spacing_ = 4.0f;
    std::string style_;

    std::vector<ItemId> items_;
    std::vector<ItemId> selected_;
    std::optional<ItemId> anchor_;
    std::unordered_map<ItemId, std::shared_ptr<Tile>> tiles_;
    std::vector<std::shared_ptr<Tile>> realized_;
    std::vector<std::size_t> realizedIndex_;
    std::shared_ptr<ScrollBar> bar_;
    std::optional<ItemId> pending_;
    DropTarget drop_;
    float offset_ = 0.0f;
    float viewport_ = 0.0f;
    std::size_t columns_ = 1;
    std::size_t first_ = 0;
};

}
