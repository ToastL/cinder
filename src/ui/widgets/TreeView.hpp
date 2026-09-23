#pragma once

#include "ui/core/Args.hpp"
#include "ui/core/Delegates.hpp"
#include "ui/core/Widget.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace cinder::ui {

class ScrollBar;
class TreeView;

using ItemId = std::uint64_t;

enum class SelectionMode : std::uint8_t { None, Single, Multi };
enum class SelectInfo : std::uint8_t { Mouse, Keyboard, Direct };
enum class DropZone : std::uint8_t { Above, Onto, Below };

class ItemDragDrop : public DragDropOperation {
public:
    ItemDragDrop(std::vector<ItemId> items, std::string label);

    const std::vector<ItemId>& items() const { return items_; }
    const std::string& label() const { return label_; }
    std::shared_ptr<Widget> decorator() const override { return decorator_; }

private:
    std::vector<ItemId> items_;
    std::string label_;
    std::shared_ptr<Widget> decorator_;
};

class TableRow : public Widget {
public:
    TableRow(TreeView& tree, ItemId item, std::shared_ptr<Widget> content);

    ItemId item() const { return item_; }
    void place(int depth, bool hasChildren);
    int depth() const { return depth_; }

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
    float expanderWidth() const;
    bool onExpander(const Geometry& geometry, glm::vec2 point) const;

    TreeView* tree_;
    ItemId item_;
    std::shared_ptr<Widget> content_;
    int depth_ = 0;
    bool hasChildren_ = false;
};

class TreeView : public Widget {
public:
    using Items = std::function<std::vector<ItemId>()>;
    using Children = std::function<std::vector<ItemId>(ItemId)>;
    using ParentOf = std::function<std::optional<ItemId>(ItemId)>;
    using GenerateRow = std::function<std::shared_ptr<Widget>(ItemId)>;
    using OnSelectionChanged = std::function<void(std::optional<ItemId>, SelectInfo)>;
    using OnItem = std::function<void(ItemId)>;
    using OnExpansionChanged = std::function<void(ItemId, bool)>;
    using ContextMenu = std::function<std::shared_ptr<Widget>(std::optional<ItemId>)>;
    using DragFrom = std::function<std::shared_ptr<DragDropOperation>(ItemId)>;
    using CanAcceptDrop = std::function<std::optional<DropZone>(const DragDropEvent&, std::optional<ItemId>, DropZone)>;
    using AcceptDrop = std::function<void(const DragDropEvent&, std::optional<ItemId>, DropZone)>;
    using KeyHandler = std::function<bool(const KeyEvent&)>;

    static constexpr float WHEEL_STEP = 3.0f;

    struct Args : ::cinder::ui::Args<Args, TreeView> {
        UI_EVENT(Items, treeItemsSource)
        UI_EVENT(Children, onGetChildren)
        UI_EVENT(ParentOf, onGetParent)
        UI_EVENT(GenerateRow, onGenerateRow)
        UI_EVENT(OnSelectionChanged, onSelectionChanged)
        UI_EVENT(OnItem, onMouseButtonDoubleClick)
        UI_EVENT(OnExpansionChanged, onExpansionChanged)
        UI_EVENT(ContextMenu, onContextMenuOpening)
        UI_EVENT(DragFrom, onDragDetected)
        UI_EVENT(CanAcceptDrop, onCanAcceptDrop)
        UI_EVENT(AcceptDrop, onAcceptDrop)
        UI_EVENT(KeyHandler, onKeyDownHandler)
        UI_EVENT(std::function<bool(ItemId)>, isItemSelected)
        UI_ARG(SelectionMode, selectionMode, SelectionMode::Single)
        UI_ARG(std::optional<float>, itemHeight)
        UI_ARG(std::string, style, "TableView")
    };

    void construct(const Args& args);

    bool isExpanded(ItemId item) const { return expanded_.count(item) != 0; }
    void setExpanded(ItemId item, bool expanded);
    void toggleExpansion(ItemId item);
    bool isSelected(ItemId item) const;
    std::vector<ItemId> selectedItems() const { return selected_; }
    void setSelection(std::optional<ItemId> item, SelectInfo how = SelectInfo::Direct);
    void clearSelection(SelectInfo how = SelectInfo::Direct);
    void requestScrollIntoView(ItemId item) { pending_ = item; }
    void reveal(ItemId item);
    void refresh() { rows_.clear(); }

    bool hasChildren(ItemId item) const;
    float rowHeight() const;
    float indent() const;
    const std::string& style() const { return style_; }
    std::size_t visibleCount() const { return flat_.size(); }
    std::size_t realizedCount() const { return realized_.size(); }
    std::optional<ItemId> itemAt(std::size_t index) const;
    const std::shared_ptr<TableRow>& rowFor(ItemId item) const;
    std::optional<DropZone> dropZoneFor(ItemId item) const;

    void clickRow(ItemId item, const PointerEvent& event);
    void doubleClickRow(ItemId item);
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
    void onDragEnter(const Geometry& geometry, const DragDropEvent& event) override;
    void onDragLeave(const DragDropEvent& event) override;
    Reply onDragOver(const Geometry& geometry, const DragDropEvent& event) override;
    Reply onDrop(const Geometry& geometry, const DragDropEvent& event) override;

protected:
    glm::vec2 computeDesiredSize(float layoutScale) const override;
    int onPaint(const PaintArgs& args, const Geometry& geometry, ElementList& list, int layer,
                const PaintStyle& style, bool enabled) const override;

private:
    struct Flat {
        ItemId item = 0;
        int depth = 0;
        bool hasChildren = false;
    };

    struct DropTarget {
        std::optional<ItemId> item;
        DropZone zone = DropZone::Onto;
        bool valid = false;
    };

    void flatten();
    void gather(ItemId item, int depth);
    void realize(const Geometry& geometry);
    std::size_t indexOf(ItemId item) const;
    std::optional<ItemId> itemAtPoint(const Geometry& geometry, glm::vec2 point, float* fraction = nullptr) const;
    void moveSelection(int delta);
    void notify(std::optional<ItemId> item, SelectInfo how);

    Items roots_;
    Children children_;
    ParentOf parent_;
    GenerateRow generate_;
    OnSelectionChanged onSelection_;
    OnItem onDoubleClick_;
    OnExpansionChanged onExpansion_;
    ContextMenu onContextMenu_;
    DragFrom onDrag_;
    CanAcceptDrop canDrop_;
    AcceptDrop acceptDrop_;
    KeyHandler onKey_;
    std::function<bool(ItemId)> selectedGetter_;
    SelectionMode mode_ = SelectionMode::Single;
    std::optional<float> height_;
    std::string style_;

    std::unordered_set<ItemId> expanded_;
    std::vector<ItemId> selected_;
    std::optional<ItemId> anchor_;
    std::vector<Flat> flat_;
    std::unordered_map<ItemId, std::shared_ptr<TableRow>> rows_;
    std::vector<std::shared_ptr<TableRow>> realized_;
    std::shared_ptr<ScrollBar> bar_;
    std::optional<ItemId> pending_;
    DropTarget drop_;
    float offset_ = 0.0f;
    float viewport_ = 0.0f;
    std::size_t first_ = 0;
};

using ListView = TreeView;

}
