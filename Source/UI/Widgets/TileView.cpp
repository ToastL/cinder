#include "UI/Widgets/TileView.hpp"

#include "UI/Core/ElementList.hpp"
#include "UI/Framework/Application.hpp"
#include "UI/Widgets/Menu.hpp"
#include "UI/Widgets/ScrollBox.hpp"

#include <algorithm>
#include <cmath>

namespace cinder::UI {

namespace keys = cinder::Platform::keys;

namespace {

const std::shared_ptr<Tile> NO_TILE;

}

Tile::Tile(TileView& view, ItemId item, std::shared_ptr<Widget> content)
    : view_(&view), item_(item), content_(std::move(content)) {}

glm::vec2 Tile::computeDesiredSize(float) const { return view_->tileSize(); }

void Tile::arrangeChildren(const Geometry& geometry, ArrangedChildren& out) const {
    if (!content_ || !takesSpace(content_->visibility())) return;
    out.push_back({content_, geometry.child({0.0f, 0.0f}, geometry.size)});
}

Reply Tile::onMouseDown(const Geometry&, const PointerEvent& event) {
    if (event.button == cinder::Platform::buttons::RIGHT) {
        if (!view_->isSelected(item_)) view_->setSelection(item_, SelectInfo::Mouse);
        return Reply::handled().setFocus(view_->shared_from_this());
    }
    if (event.button != cinder::Platform::buttons::LEFT) return Reply::unhandled();
    view_->clickTile(item_);
    Reply reply = Reply::handled().setFocus(view_->shared_from_this());
    return reply.detectDrag(shared_from_this(), cinder::Platform::buttons::LEFT);
}

Reply Tile::onMouseUp(const Geometry&, const PointerEvent& event) {
    if (event.button != cinder::Platform::buttons::RIGHT) return Reply::unhandled();
    view_->openContextMenu(item_, event.position);
    return Reply::handled();
}

Reply Tile::onMouseDoubleClick(const Geometry&, const PointerEvent& event) {
    if (event.button != cinder::Platform::buttons::LEFT) return Reply::unhandled();
    view_->clickTile(item_);
    view_->doubleClickTile(item_);
    return Reply::handled();
}

Reply Tile::onDragDetected(const Geometry&, const PointerEvent&) {
    std::shared_ptr<DragDropOperation> operation = view_->dragFrom(item_);
    if (!operation) return Reply::unhandled();
    return Reply::handled().beginDragDrop(std::move(operation));
}

int Tile::onPaint(const PaintArgs& args, const Geometry& geometry, ElementList& list, int layer,
                  const PaintStyle& style, bool enabled) const {
    Application& app = Application::get();
    const TableViewStyle& look = app.theme().get<TableViewStyle>(view_->style());
    const Rect rect = geometry.rect();
    if (view_->isSelected(item_)) {
        const bool active = app.focusWithin(*view_);
        (active ? look.rowSelected : look.rowSelectedInactive).paint(list, layer, rect, style, geometry.scale);
    } else if (isHovered()) {
        look.rowHovered.paint(list, layer, rect, style, geometry.scale);
    }
    const int highest = paintChildren(args, geometry, list, layer + 1, style, enabled);
    if (!view_->dropTargetIs(item_)) return highest;
    list.box(highest + 1, rect,
             BoxStyle{Color::transparent(), style.apply(look.dropTarget), 1.5f * geometry.scale, glm::vec4(3.0f)});
    return highest + 1;
}

void TileView::construct(const Args& args) {
    source_ = args.itemsSource_;
    generate_ = args.onGenerateTile_;
    onSelection_ = args.onSelectionChanged_;
    onDoubleClick_ = args.onMouseButtonDoubleClick_;
    onContextMenu_ = args.onContextMenuOpening_;
    onDrag_ = args.onDragDetected_;
    canDrop_ = args.onCanAcceptDrop_;
    acceptDrop_ = args.onAcceptDrop_;
    onKey_ = args.onKeyDownHandler_;
    selectedGetter_ = args.isItemSelected_;
    mode_ = args.selectionMode_;
    tileSize_ = glm::max(args.tileSize_, glm::vec2(1.0f));
    spacing_ = std::max(0.0f, args.spacing_);
    style_ = args.style_;
    bar_ = make<ScrollBar>().onUserScrolled([this](float fraction) { offset_ = fraction * contentHeight(); });
}

bool TileView::isSelected(ItemId item) const {
    if (selectedGetter_) return selectedGetter_(item);
    return std::find(selected_.begin(), selected_.end(), item) != selected_.end();
}

void TileView::setSelection(std::optional<ItemId> item, SelectInfo how) {
    if (!selectedGetter_) {
        selected_.clear();
        if (item) selected_.push_back(*item);
    }
    anchor_ = item;
    if (item) requestScrollIntoView(*item);
    if (onSelection_) onSelection_(item, how);
}

void TileView::clearSelection(SelectInfo how) {
    if (!selectedGetter_) selected_.clear();
    anchor_.reset();
    if (onSelection_) onSelection_(std::nullopt, how);
}

void TileView::clickTile(ItemId item) {
    if (mode_ == SelectionMode::None) return;
    setSelection(item, SelectInfo::Mouse);
}

void TileView::doubleClickTile(ItemId item) {
    if (onDoubleClick_) onDoubleClick_(item);
}

void TileView::openContextMenu(std::optional<ItemId> item, glm::vec2 at) {
    if (!onContextMenu_) return;
    if (std::shared_ptr<Widget> menu = onContextMenu_(item)) showContextMenu(std::move(menu), at);
}

std::shared_ptr<DragDropOperation> TileView::dragFrom(ItemId item) { return onDrag_ ? onDrag_(item) : nullptr; }

std::optional<ItemId> TileView::itemAt(std::size_t index) const {
    if (index >= items_.size()) return std::nullopt;
    return items_[index];
}

const std::shared_ptr<Tile>& TileView::tileFor(ItemId item) const {
    const auto found = tiles_.find(item);
    return found == tiles_.end() ? NO_TILE : found->second;
}

std::size_t TileView::indexOf(ItemId item) const {
    const auto found = std::find(items_.begin(), items_.end(), item);
    return static_cast<std::size_t>(found - items_.begin());
}

std::size_t TileView::columnsFor(float width) const {
    const float usable = std::max(0.0f, width + spacing_);
    return std::max<std::size_t>(1, static_cast<std::size_t>(std::floor(usable / pitchX())));
}

std::size_t TileView::rowCount() const { return (items_.size() + columns_ - 1) / columns_; }

float TileView::contentHeight() const {
    const std::size_t rows = rowCount();
    return rows == 0 ? 0.0f : static_cast<float>(rows) * pitchY() - spacing_;
}

void TileView::realize(const Geometry& geometry) {
    items_ = source_ ? source_() : std::vector<ItemId>{};
    viewport_ = geometry.size.y;
    columns_ = columnsFor(geometry.size.x);
    if (contentHeight() > viewport_ + 0.5f) columns_ = columnsFor(geometry.size.x - bar_->thickness());
    const float content = contentHeight();

    if (pending_) {
        const std::size_t index = indexOf(*pending_);
        if (index < items_.size()) {
            const float top = static_cast<float>(index / columns_) * pitchY();
            if (top < offset_) offset_ = top;
            if (top + tileSize_.y > offset_ + viewport_) offset_ = top + tileSize_.y - viewport_;
        }
        pending_.reset();
    }
    offset_ = std::clamp(offset_, 0.0f, std::max(0.0f, content - viewport_));

    const auto firstRow = static_cast<std::size_t>(std::max(0.0f, std::floor(offset_ / pitchY())));
    const auto rows = static_cast<std::size_t>(std::ceil(std::max(0.0f, viewport_) / pitchY())) + 1;
    first_ = std::min(items_.size(), firstRow * columns_);
    const std::size_t last = std::min(items_.size(), (firstRow + rows) * columns_);

    const float scale = Application::current() != nullptr ? Application::current()->scale() : 1.0f;
    std::unordered_map<ItemId, std::shared_ptr<Tile>> next;
    realized_.clear();
    realizedIndex_.clear();
    for (std::size_t i = first_; i < last; ++i) {
        const ItemId item = items_[i];
        std::shared_ptr<Tile> tile;
        const auto found = tiles_.find(item);
        if (found != tiles_.end()) tile = found->second;
        else if (generate_) tile = std::make_shared<Tile>(*this, item, generate_(item));
        if (!tile) continue;
        tile->prepass(scale);
        next.emplace(item, tile);
        realized_.push_back(tile);
        realizedIndex_.push_back(i);
    }
    tiles_ = std::move(next);
    bar_->setState(content > 0.0f ? offset_ / content : 0.0f, content > 0.0f ? std::min(1.0f, viewport_ / content) : 1.0f);
}

void TileView::tick(const Geometry& geometry, double, float) { realize(geometry); }

int TileView::childCount() const { return static_cast<int>(realized_.size()) + 1; }

Widget* TileView::childAt(int index) const {
    if (index < static_cast<int>(realized_.size())) return realized_[static_cast<std::size_t>(index)].get();
    return bar_.get();
}

void TileView::arrangeChildren(const Geometry& geometry, ArrangedChildren& out) const {
    const bool scrolling = contentHeight() > geometry.size.y + 0.5f;
    const float thickness = scrolling ? bar_->thickness() : 0.0f;
    for (std::size_t i = 0; i < realized_.size(); ++i) {
        const std::size_t index = realizedIndex_[i];
        const float x = static_cast<float>(index % columns_) * pitchX();
        const float y = static_cast<float>(index / columns_) * pitchY() - offset_;
        out.push_back({realized_[i], geometry.child({x, y}, tileSize_)});
    }
    if (scrolling) {
        const float left = std::max(0.0f, geometry.size.x - thickness);
        out.push_back({bar_, geometry.child({left, 0.0f}, {thickness, geometry.size.y})});
    }
}

glm::vec2 TileView::computeDesiredSize(float) const { return {tileSize_.x, contentHeight()}; }

std::optional<ItemId> TileView::itemAtPoint(const Geometry& geometry, glm::vec2 point) const {
    const glm::vec2 local = geometry.local(point) + glm::vec2(0.0f, offset_);
    if (local.x < 0.0f || local.y < 0.0f) return std::nullopt;
    const auto column = static_cast<std::size_t>(std::floor(local.x / pitchX()));
    const auto row = static_cast<std::size_t>(std::floor(local.y / pitchY()));
    if (column >= columns_) return std::nullopt;
    if (local.x - static_cast<float>(column) * pitchX() > tileSize_.x) return std::nullopt;
    if (local.y - static_cast<float>(row) * pitchY() > tileSize_.y) return std::nullopt;
    const std::size_t index = row * columns_ + column;
    if (index >= items_.size()) return std::nullopt;
    return items_[index];
}

Reply TileView::onMouseDown(const Geometry&, const PointerEvent& event) {
    if (event.button == cinder::Platform::buttons::LEFT) {
        clearSelection(SelectInfo::Mouse);
        return Reply::handled().setFocus(shared_from_this());
    }
    if (event.button == cinder::Platform::buttons::RIGHT) return Reply::handled().setFocus(shared_from_this());
    return Reply::unhandled();
}

Reply TileView::onMouseUp(const Geometry&, const PointerEvent& event) {
    if (event.button != cinder::Platform::buttons::RIGHT) return Reply::unhandled();
    openContextMenu(std::nullopt, event.position);
    return Reply::handled();
}

Reply TileView::onMouseWheel(const Geometry&, const PointerEvent& event) {
    const float before = offset_;
    offset_ = std::clamp(offset_ - event.wheel.y * pitchY() * WHEEL_STEP, 0.0f,
                         std::max(0.0f, contentHeight() - viewport_));
    return offset_ != before ? Reply::handled() : Reply::unhandled();
}

std::size_t TileView::currentIndex() const {
    const std::size_t anchored = anchor_ && !selectedGetter_ ? indexOf(*anchor_) : items_.size();
    if (anchored < items_.size()) return anchored;
    for (std::size_t i = 0; i < items_.size(); ++i) {
        if (isSelected(items_[i])) return i;
    }
    return items_.size();
}

void TileView::moveSelection(std::ptrdiff_t delta) {
    if (items_.empty() || mode_ == SelectionMode::None) return;
    const std::size_t index = currentIndex();
    std::size_t next = 0;
    if (index >= items_.size()) {
        next = delta > 0 ? 0 : items_.size() - 1;
    } else {
        const auto target = static_cast<std::ptrdiff_t>(index) + delta;
        if (target < 0 || target >= static_cast<std::ptrdiff_t>(items_.size())) return;
        next = static_cast<std::size_t>(target);
    }
    setSelection(items_[next], SelectInfo::Keyboard);
}

Reply TileView::onKeyDown(const Geometry&, const KeyEvent& event) {
    if (onKey_ && onKey_(event)) return Reply::handled();
    const auto row = static_cast<std::ptrdiff_t>(columns_);
    switch (event.key) {
        case keys::LEFT:
            moveSelection(-1);
            return Reply::handled();
        case keys::RIGHT:
            moveSelection(1);
            return Reply::handled();
        case keys::UP:
            moveSelection(-row);
            return Reply::handled();
        case keys::DOWN:
            moveSelection(row);
            return Reply::handled();
        case keys::HOME:
            if (!items_.empty()) setSelection(items_.front(), SelectInfo::Keyboard);
            return Reply::handled();
        case keys::END:
            if (!items_.empty()) setSelection(items_.back(), SelectInfo::Keyboard);
            return Reply::handled();
        case keys::ENTER:
        case keys::KEYPAD_ENTER: {
            const std::size_t index = currentIndex();
            if (index < items_.size()) doubleClickTile(items_[index]);
            return Reply::handled();
        }
        default:
            return Reply::unhandled();
    }
}

void TileView::onDragLeave(const DragDropEvent&) { drop_ = DropTarget{}; }

Reply TileView::onDragOver(const Geometry& geometry, const DragDropEvent& event) {
    if (!canDrop_) return Reply::unhandled();
    const std::optional<ItemId> item = itemAtPoint(geometry, event.position);
    const bool allowed = canDrop_(event, item);
    drop_ = DropTarget{item, allowed};
    return allowed ? Reply::handled() : Reply::unhandled();
}

Reply TileView::onDrop(const Geometry& geometry, const DragDropEvent& event) {
    drop_ = DropTarget{};
    if (!canDrop_ || !acceptDrop_) return Reply::unhandled();
    const std::optional<ItemId> item = itemAtPoint(geometry, event.position);
    if (!canDrop_(event, item)) return Reply::unhandled();
    acceptDrop_(event, item);
    return Reply::handled();
}

int TileView::onPaint(const PaintArgs& args, const Geometry& geometry, ElementList& list, int layer,
                      const PaintStyle& style, bool enabled) const {
    const TableViewStyle& look = Application::get().theme().get<TableViewStyle>(style_);
    look.background.paint(list, layer, geometry.rect(), style, geometry.scale);

    ArrangedChildren arranged;
    arrangeChildren(geometry, arranged);
    int highest = layer;
    list.pushClip(geometry.rect());
    for (const ArrangedWidget& child : arranged) {
        if (child.widget == bar_) continue;
        highest = std::max(highest, child.widget->paint(args, child.geometry, list, layer + 1, style, enabled));
    }
    if (drop_.valid && !drop_.item) {
        list.box(highest + 1, geometry.rect(),
                 BoxStyle{Color::transparent(), style.apply(look.dropTarget), 1.5f * geometry.scale, glm::vec4(2.0f)});
        ++highest;
    }
    for (const ArrangedWidget& child : arranged) {
        if (child.widget == bar_) highest = child.widget->paint(args, child.geometry, list, highest + 1, style, enabled);
    }
    list.popClip();
    return highest;
}

}
