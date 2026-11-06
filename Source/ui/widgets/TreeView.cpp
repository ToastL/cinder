#include "ui/widgets/TreeView.hpp"

#include "ui/core/ElementList.hpp"
#include "ui/framework/Application.hpp"
#include "ui/widgets/Border.hpp"
#include "ui/widgets/ExpandableArea.hpp"
#include "ui/widgets/Label.hpp"
#include "ui/widgets/Menu.hpp"
#include "ui/widgets/ScrollBox.hpp"

#include <algorithm>
#include <cmath>

namespace cinder::ui {

namespace keys = cinder::platform::keys;

namespace {

const std::shared_ptr<TableRow> NO_ROW;

}

ItemDragDrop::ItemDragDrop(std::vector<ItemId> items, std::string label)
    : items_(std::move(items)), label_(std::move(label)) {
    decorator_ = make<Border>()
            .brush([] { return Application::get().theme().get<MenuStyle>("Menu").background; })
            .padding(Margin(8.0f, 4.0f))
            [make<Label>().text(label_)];
}

TableRow::TableRow(TreeView& tree, ItemId item, std::shared_ptr<Widget> content)
    : tree_(&tree), item_(item), content_(std::move(content)) {}

void TableRow::place(int depth, bool hasChildren) {
    depth_ = depth;
    hasChildren_ = hasChildren;
}

float TableRow::expanderWidth() const {
    const float indent = tree_->indent();
    return indent <= 0.0f ? 0.0f : static_cast<float>(depth_ + 1) * indent;
}

bool TableRow::onExpander(const Geometry& geometry, glm::vec2 point) const {
    if (!hasChildren_) return false;
    const float indent = tree_->indent();
    const float x = geometry.local(point).x;
    return indent > 0.0f && x >= static_cast<float>(depth_) * indent && x < static_cast<float>(depth_ + 1) * indent;
}

glm::vec2 TableRow::computeDesiredSize(float) const {
    const TableViewStyle& look = Application::get().theme().get<TableViewStyle>(tree_->style());
    const glm::vec2 content = content_ ? content_->desiredSize() : glm::vec2(0.0f);
    return {expanderWidth() + content.x + look.rowPadding.total().x, tree_->rowHeight()};
}

void TableRow::arrangeChildren(const Geometry& geometry, ArrangedChildren& out) const {
    if (!content_ || !takesSpace(content_->visibility())) return;
    const TableViewStyle& look = Application::get().theme().get<TableViewStyle>(tree_->style());
    const float left = expanderWidth() + look.rowPadding.left;
    const float width = std::max(0.0f, geometry.size.x - left - look.rowPadding.right);
    const float height = std::min(content_->desiredSize().y, geometry.size.y);
    out.push_back({content_, geometry.child({left, std::round((geometry.size.y - height) * 0.5f)}, {width, height})});
}

Reply TableRow::onMouseDown(const Geometry& geometry, const PointerEvent& event) {
    if (event.button == cinder::platform::buttons::RIGHT) {
        if (!tree_->isSelected(item_)) tree_->setSelection(item_, SelectInfo::Mouse);
        return Reply::handled().setFocus(tree_->shared_from_this());
    }
    if (event.button != cinder::platform::buttons::LEFT) return Reply::unhandled();
    if (onExpander(geometry, event.position)) {
        tree_->toggleExpansion(item_);
        return Reply::handled();
    }
    tree_->clickRow(item_, event);
    Reply reply = Reply::handled().setFocus(tree_->shared_from_this());
    return reply.detectDrag(shared_from_this(), cinder::platform::buttons::LEFT);
}

Reply TableRow::onMouseUp(const Geometry&, const PointerEvent& event) {
    if (event.button != cinder::platform::buttons::RIGHT) return Reply::unhandled();
    tree_->openContextMenu(item_, event.position);
    return Reply::handled();
}

Reply TableRow::onMouseDoubleClick(const Geometry& geometry, const PointerEvent& event) {
    if (event.button != cinder::platform::buttons::LEFT) return Reply::unhandled();
    if (onExpander(geometry, event.position)) return onMouseDown(geometry, event);
    tree_->clickRow(item_, event);
    tree_->doubleClickRow(item_);
    return Reply::handled();
}

Reply TableRow::onDragDetected(const Geometry&, const PointerEvent&) {
    std::shared_ptr<DragDropOperation> operation = tree_->dragFrom(item_);
    if (!operation) return Reply::unhandled();
    return Reply::handled().beginDragDrop(std::move(operation));
}

int TableRow::onPaint(const PaintArgs& args, const Geometry& geometry, ElementList& list, int layer,
                      const PaintStyle& style, bool enabled) const {
    Application& app = Application::get();
    const TableViewStyle& look = app.theme().get<TableViewStyle>(tree_->style());
    const Rect rect = geometry.rect();
    if (tree_->isSelected(item_)) {
        const bool active = app.focusWithin(*tree_);
        (active ? look.rowSelected : look.rowSelectedInactive).paint(list, layer, rect, style, geometry.scale);
    } else if (isHovered()) {
        look.rowHovered.paint(list, layer, rect, style, geometry.scale);
    }

    if (hasChildren_ && look.indent > 0.0f) {
        const glm::vec2 centre = geometry.absolute({(static_cast<float>(depth_) + 0.5f) * look.indent, geometry.size.y * 0.5f});
        Color arrow = style.apply(style.foreground);
        if (!isHovered()) arrow.a *= 0.75f;
        ExpanderArrow::paintArrow(list, layer + 1, centre, 9.0f * geometry.scale, tree_->isExpanded(item_), arrow);
    }

    const int highest = paintChildren(args, geometry, list, layer + 1, style, enabled);

    if (const std::optional<DropZone> zone = tree_->dropZoneFor(item_)) {
        const Color mark = style.apply(look.dropTarget);
        if (*zone == DropZone::Onto) {
            list.box(highest + 1, rect, BoxStyle{Color::transparent(), mark, 1.5f * geometry.scale, glm::vec4(2.0f)});
        } else {
            const float y = *zone == DropZone::Above ? 0.0f : geometry.size.y - 2.0f;
            list.box(highest + 1, geometry.localRect({0.0f, y}, {geometry.size.x, 2.0f}), BoxStyle{mark});
        }
        return highest + 1;
    }
    return highest;
}

void TreeView::construct(const Args& args) {
    roots_ = args.treeItemsSource_;
    children_ = args.onGetChildren_;
    parent_ = args.onGetParent_;
    generate_ = args.onGenerateRow_;
    onSelection_ = args.onSelectionChanged_;
    onDoubleClick_ = args.onMouseButtonDoubleClick_;
    onExpansion_ = args.onExpansionChanged_;
    onContextMenu_ = args.onContextMenuOpening_;
    onDrag_ = args.onDragDetected_;
    canDrop_ = args.onCanAcceptDrop_;
    acceptDrop_ = args.onAcceptDrop_;
    onKey_ = args.onKeyDownHandler_;
    selectedGetter_ = args.isItemSelected_;
    mode_ = args.selectionMode_;
    height_ = args.itemHeight_;
    style_ = args.style_;
    bar_ = make<ScrollBar>().onUserScrolled([this](float fraction) {
        offset_ = fraction * static_cast<float>(flat_.size()) * rowHeight();
    });
}

float TreeView::rowHeight() const {
    if (height_) return *height_;
    return Application::get().theme().get<TableViewStyle>(style_).rowHeight;
}

float TreeView::indent() const {
    if (!children_) return 0.0f;
    return Application::get().theme().get<TableViewStyle>(style_).indent;
}

bool TreeView::hasChildren(ItemId item) const { return children_ && !children_(item).empty(); }

void TreeView::setExpanded(ItemId item, bool expanded) {
    const bool was = isExpanded(item);
    if (was == expanded) return;
    if (expanded) expanded_.insert(item);
    else expanded_.erase(item);
    if (onExpansion_) onExpansion_(item, expanded);
}

void TreeView::toggleExpansion(ItemId item) { setExpanded(item, !isExpanded(item)); }

bool TreeView::isSelected(ItemId item) const {
    if (selectedGetter_) return selectedGetter_(item);
    return std::find(selected_.begin(), selected_.end(), item) != selected_.end();
}

void TreeView::notify(std::optional<ItemId> item, SelectInfo how) {
    if (onSelection_) onSelection_(item, how);
}

void TreeView::setSelection(std::optional<ItemId> item, SelectInfo how) {
    if (!selectedGetter_) {
        selected_.clear();
        if (item) selected_.push_back(*item);
    }
    anchor_ = item;
    if (item) requestScrollIntoView(*item);
    notify(item, how);
}

void TreeView::clearSelection(SelectInfo how) {
    if (!selectedGetter_) selected_.clear();
    anchor_.reset();
    notify(std::nullopt, how);
}

void TreeView::clickRow(ItemId item, const PointerEvent& event) {
    if (mode_ == SelectionMode::None) return;
    if (mode_ == SelectionMode::Multi && !selectedGetter_) {
        if (event.primary()) {
            const auto found = std::find(selected_.begin(), selected_.end(), item);
            if (found == selected_.end()) selected_.push_back(item);
            else selected_.erase(found);
            anchor_ = item;
            notify(item, SelectInfo::Mouse);
            return;
        }
        if (event.shift() && anchor_) {
            const std::size_t from = indexOf(*anchor_);
            const std::size_t to = indexOf(item);
            if (from < flat_.size() && to < flat_.size()) {
                selected_.clear();
                for (std::size_t i = std::min(from, to); i <= std::max(from, to); ++i) selected_.push_back(flat_[i].item);
                notify(item, SelectInfo::Mouse);
                return;
            }
        }
    }
    setSelection(item, SelectInfo::Mouse);
}

void TreeView::doubleClickRow(ItemId item) {
    if (onDoubleClick_) {
        onDoubleClick_(item);
        return;
    }
    if (hasChildren(item)) toggleExpansion(item);
}

void TreeView::openContextMenu(std::optional<ItemId> item, glm::vec2 at) {
    if (!onContextMenu_) return;
    if (std::shared_ptr<Widget> menu = onContextMenu_(item)) showContextMenu(std::move(menu), at);
}

std::shared_ptr<DragDropOperation> TreeView::dragFrom(ItemId item) {
    return onDrag_ ? onDrag_(item) : nullptr;
}

void TreeView::reveal(ItemId item) {
    if (parent_) {
        std::optional<ItemId> at = parent_(item);
        int guard = 0;
        while (at && guard++ < 256) {
            setExpanded(*at, true);
            at = parent_(*at);
        }
    }
    requestScrollIntoView(item);
}

std::size_t TreeView::indexOf(ItemId item) const {
    for (std::size_t i = 0; i < flat_.size(); ++i) {
        if (flat_[i].item == item) return i;
    }
    return flat_.size();
}

std::optional<ItemId> TreeView::itemAt(std::size_t index) const {
    if (index >= flat_.size()) return std::nullopt;
    return flat_[index].item;
}

const std::shared_ptr<TableRow>& TreeView::rowFor(ItemId item) const {
    const auto found = rows_.find(item);
    return found == rows_.end() ? NO_ROW : found->second;
}

std::optional<DropZone> TreeView::dropZoneFor(ItemId item) const {
    if (!drop_.valid || !drop_.item || *drop_.item != item) return std::nullopt;
    return drop_.zone;
}

void TreeView::gather(ItemId item, int depth) {
    const std::vector<ItemId> kids = children_ ? children_(item) : std::vector<ItemId>{};
    flat_.push_back(Flat{item, depth, !kids.empty()});
    if (kids.empty() || !isExpanded(item)) return;
    for (const ItemId child : kids) gather(child, depth + 1);
}

void TreeView::flatten() {
    flat_.clear();
    if (!roots_) return;
    for (const ItemId item : roots_()) gather(item, 0);
}

void TreeView::realize(const Geometry& geometry) {
    const float height = std::max(1.0f, rowHeight());
    viewport_ = geometry.size.y;
    const float content = static_cast<float>(flat_.size()) * height;

    if (pending_) {
        const std::size_t index = indexOf(*pending_);
        if (index < flat_.size()) {
            const float top = static_cast<float>(index) * height;
            if (top < offset_) offset_ = top;
            if (top + height > offset_ + viewport_) offset_ = top + height - viewport_;
        }
        pending_.reset();
    }
    offset_ = std::clamp(offset_, 0.0f, std::max(0.0f, content - viewport_));

    first_ = static_cast<std::size_t>(std::max(0.0f, std::floor(offset_ / height)));
    const auto span = static_cast<std::size_t>(std::ceil(std::max(0.0f, viewport_) / height)) + 1;
    const std::size_t last = std::min(flat_.size(), first_ + span);

    const float scale = Application::current() != nullptr ? Application::current()->scale() : 1.0f;
    std::unordered_map<ItemId, std::shared_ptr<TableRow>> next;
    realized_.clear();
    for (std::size_t i = first_; i < last; ++i) {
        const Flat& flat = flat_[i];
        std::shared_ptr<TableRow> row;
        const auto found = rows_.find(flat.item);
        if (found != rows_.end()) row = found->second;
        else if (generate_) row = std::make_shared<TableRow>(*this, flat.item, generate_(flat.item));
        if (!row) continue;
        row->place(flat.depth, flat.hasChildren);
        row->prepass(scale);
        next.emplace(flat.item, row);
        realized_.push_back(row);
    }
    rows_ = std::move(next);
    bar_->setState(content > 0.0f ? offset_ / content : 0.0f,
                   content > 0.0f ? std::min(1.0f, viewport_ / content) : 1.0f);
}

void TreeView::tick(const Geometry& geometry, double, float) {
    flatten();
    realize(geometry);
}

int TreeView::childCount() const { return static_cast<int>(realized_.size()) + 1; }

Widget* TreeView::childAt(int index) const {
    if (index < static_cast<int>(realized_.size())) return realized_[static_cast<std::size_t>(index)].get();
    return bar_.get();
}

void TreeView::arrangeChildren(const Geometry& geometry, ArrangedChildren& out) const {
    const float height = std::max(1.0f, rowHeight());
    const float content = static_cast<float>(flat_.size()) * height;
    const bool scrolling = content > geometry.size.y + 0.5f;
    const float thickness = scrolling ? bar_->thickness() : 0.0f;
    const float width = std::max(0.0f, geometry.size.x - thickness);
    for (std::size_t i = 0; i < realized_.size(); ++i) {
        const float y = static_cast<float>(first_ + i) * height - offset_;
        out.push_back({realized_[i], geometry.child({0.0f, y}, {width, height})});
    }
    if (scrolling) out.push_back({bar_, geometry.child({width, 0.0f}, {thickness, geometry.size.y})});
}

glm::vec2 TreeView::computeDesiredSize(float) const {
    float width = 0.0f;
    for (const std::shared_ptr<TableRow>& row : realized_) width = std::max(width, row->desiredSize().x);
    return {width, static_cast<float>(flat_.size()) * std::max(1.0f, rowHeight())};
}

std::optional<ItemId> TreeView::itemAtPoint(const Geometry& geometry, glm::vec2 point, float* fraction) const {
    const float height = std::max(1.0f, rowHeight());
    const float y = geometry.local(point).y + offset_;
    if (y < 0.0f) return std::nullopt;
    const auto index = static_cast<std::size_t>(std::floor(y / height));
    if (index >= flat_.size()) return std::nullopt;
    if (fraction != nullptr) *fraction = (y - static_cast<float>(index) * height) / height;
    return flat_[index].item;
}

Reply TreeView::onMouseDown(const Geometry&, const PointerEvent& event) {
    if (event.button == cinder::platform::buttons::LEFT) {
        clearSelection(SelectInfo::Mouse);
        return Reply::handled().setFocus(shared_from_this());
    }
    if (event.button == cinder::platform::buttons::RIGHT) return Reply::handled().setFocus(shared_from_this());
    return Reply::unhandled();
}

Reply TreeView::onMouseUp(const Geometry&, const PointerEvent& event) {
    if (event.button != cinder::platform::buttons::RIGHT) return Reply::unhandled();
    openContextMenu(std::nullopt, event.position);
    return Reply::handled();
}

Reply TreeView::onMouseWheel(const Geometry&, const PointerEvent& event) {
    const float height = std::max(1.0f, rowHeight());
    const float content = static_cast<float>(flat_.size()) * height;
    const float before = offset_;
    offset_ = std::clamp(offset_ - event.wheel.y * height * WHEEL_STEP, 0.0f,
                         std::max(0.0f, content - viewport_));
    return offset_ != before ? Reply::handled() : Reply::unhandled();
}

void TreeView::moveSelection(int delta) {
    if (flat_.empty()) return;
    std::size_t index = anchor_ ? indexOf(*anchor_) : flat_.size();
    if (index >= flat_.size()) {
        for (std::size_t i = 0; i < flat_.size(); ++i) {
            if (isSelected(flat_[i].item)) {
                index = i;
                break;
            }
        }
    }
    std::size_t next = 0;
    if (index >= flat_.size()) next = delta > 0 ? 0 : flat_.size() - 1;
    else if (delta < 0) next = index == 0 ? 0 : index - 1;
    else next = std::min(flat_.size() - 1, index + 1);
    setSelection(flat_[next].item, SelectInfo::Keyboard);
}

Reply TreeView::onKeyDown(const Geometry&, const KeyEvent& event) {
    if (onKey_ && onKey_(event)) return Reply::handled();
    const std::size_t index = anchor_ ? indexOf(*anchor_) : flat_.size();
    switch (event.key) {
        case keys::UP:
            moveSelection(-1);
            return Reply::handled();
        case keys::DOWN:
            moveSelection(1);
            return Reply::handled();
        case keys::HOME:
            if (!flat_.empty()) setSelection(flat_.front().item, SelectInfo::Keyboard);
            return Reply::handled();
        case keys::END:
            if (!flat_.empty()) setSelection(flat_.back().item, SelectInfo::Keyboard);
            return Reply::handled();
        case keys::LEFT:
            if (index < flat_.size()) {
                const Flat& flat = flat_[index];
                if (flat.hasChildren && isExpanded(flat.item)) setExpanded(flat.item, false);
                else if (parent_) {
                    if (const std::optional<ItemId> up = parent_(flat.item)) setSelection(*up, SelectInfo::Keyboard);
                }
            }
            return Reply::handled();
        case keys::RIGHT:
            if (index < flat_.size()) {
                const Flat& flat = flat_[index];
                if (flat.hasChildren && !isExpanded(flat.item)) setExpanded(flat.item, true);
                else if (flat.hasChildren && index + 1 < flat_.size()) setSelection(flat_[index + 1].item, SelectInfo::Keyboard);
            }
            return Reply::handled();
        case keys::ENTER:
        case keys::KEYPAD_ENTER:
            if (index < flat_.size()) doubleClickRow(flat_[index].item);
            return Reply::handled();
        default:
            return Reply::unhandled();
    }
}

void TreeView::onDragEnter(const Geometry&, const DragDropEvent&) {}

void TreeView::onDragLeave(const DragDropEvent&) { drop_ = DropTarget{}; }

Reply TreeView::onDragOver(const Geometry& geometry, const DragDropEvent& event) {
    if (!canDrop_) return Reply::unhandled();
    float fraction = 0.5f;
    const std::optional<ItemId> item = itemAtPoint(geometry, event.position, &fraction);
    DropZone zone = DropZone::Onto;
    if (item) zone = fraction < 0.3f ? DropZone::Above : (fraction > 0.7f ? DropZone::Below : DropZone::Onto);
    const std::optional<DropZone> allowed = canDrop_(event, item, zone);
    drop_ = DropTarget{item, allowed.value_or(zone), allowed.has_value()};
    return allowed ? Reply::handled() : Reply::unhandled();
}

Reply TreeView::onDrop(const Geometry& geometry, const DragDropEvent& event) {
    if (!canDrop_ || !acceptDrop_) return Reply::unhandled();
    float fraction = 0.5f;
    const std::optional<ItemId> item = itemAtPoint(geometry, event.position, &fraction);
    DropZone zone = DropZone::Onto;
    if (item) zone = fraction < 0.3f ? DropZone::Above : (fraction > 0.7f ? DropZone::Below : DropZone::Onto);
    const std::optional<DropZone> allowed = canDrop_(event, item, zone);
    drop_ = DropTarget{};
    if (!allowed) return Reply::unhandled();
    acceptDrop_(event, item, *allowed);
    return Reply::handled();
}

int TreeView::onPaint(const PaintArgs& args, const Geometry& geometry, ElementList& list, int layer,
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
