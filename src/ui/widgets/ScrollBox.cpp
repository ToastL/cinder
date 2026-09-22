#include "ui/widgets/ScrollBox.hpp"

#include "ui/core/ElementList.hpp"
#include "ui/framework/Application.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace cinder::ui {

namespace {

bool present(const std::shared_ptr<Widget>& widget) { return widget && takesSpace(widget->visibility()); }

}

void ScrollBar::construct(const Args& args) {
    orientation_ = args.orientation_;
    onScrolled_ = args.onUserScrolled_;
    style_ = args.style_;
}

void ScrollBar::setState(float offset, float thumb) {
    thumb_ = std::clamp(thumb, 0.0f, 1.0f);
    offset_ = std::clamp(offset, 0.0f, 1.0f - thumb_);
}

float ScrollBar::thickness() const { return Application::get().theme().get<ScrollBarStyle>(style_).thickness; }

glm::vec2 ScrollBar::computeDesiredSize(float) const {
    return orientation_ == Orientation::Vertical ? glm::vec2(thickness(), 0.0f) : glm::vec2(0.0f, thickness());
}

Span ScrollBar::thumbSpan(const Geometry& geometry) const {
    const ScrollBarStyle& look = Application::get().theme().get<ScrollBarStyle>(style_);
    const float track = along(geometry.size);
    const float length = std::min(track, std::max(look.minThumb, track * thumb_));
    const float free = 1.0f - thumb_;
    const float position = free > 0.0f ? offset_ / free * (track - length) : 0.0f;
    return {position, length};
}

Reply ScrollBar::onMouseDown(const Geometry& geometry, const PointerEvent& event) {
    if (event.button != cinder::platform::buttons::LEFT || !needed()) return Reply::unhandled();
    const float point = along(geometry.local(event.position));
    const Span thumb = thumbSpan(geometry);
    if (point >= thumb.offset && point <= thumb.offset + thumb.size) {
        dragging_ = true;
        grab_ = point - thumb.offset;
        return Reply::handled().captureMouse(shared_from_this());
    }
    const float next = std::clamp(offset_ + (point < thumb.offset ? -thumb_ : thumb_), 0.0f, 1.0f - thumb_);
    if (onScrolled_) onScrolled_(next);
    return Reply::handled();
}

Reply ScrollBar::onMouseMove(const Geometry& geometry, const PointerEvent& event) {
    if (!dragging_) return Reply::unhandled();
    const Span thumb = thumbSpan(geometry);
    const float travel = along(geometry.size) - thumb.size;
    if (travel <= 0.0f) return Reply::handled();
    const float position = along(geometry.local(event.position)) - grab_;
    const float next = std::clamp(position / travel * (1.0f - thumb_), 0.0f, 1.0f - thumb_);
    if (onScrolled_) onScrolled_(next);
    return Reply::handled();
}

Reply ScrollBar::onMouseUp(const Geometry&, const PointerEvent& event) {
    if (!dragging_ || event.button != cinder::platform::buttons::LEFT) return Reply::unhandled();
    dragging_ = false;
    return Reply::handled().releaseMouseCapture();
}

int ScrollBar::onPaint(const PaintArgs&, const Geometry& geometry, ElementList& list, int layer,
                        const PaintStyle& style, bool) const {
    if (!needed()) return layer;
    const ScrollBarStyle& look = Application::get().theme().get<ScrollBarStyle>(style_);
    look.track.paint(list, layer, geometry.rect(), style, geometry.scale);
    const Span thumb = thumbSpan(geometry);
    const float inset = 2.0f;
    const Rect rect = orientation_ == Orientation::Vertical
            ? geometry.localRect({inset, thumb.offset}, {geometry.size.x - inset * 2.0f, thumb.size})
            : geometry.localRect({thumb.offset, inset}, {thumb.size, geometry.size.y - inset * 2.0f});
    const Brush& brush = dragging_ ? look.thumbDragged : (isHovered() ? look.thumbHovered : look.thumb);
    brush.paint(list, layer + 1, rect, style, geometry.scale);
    return layer + 1;
}

void ScrollBox::construct(const Args& args) {
    slots_ = args.slots_;
    onScrolled_ = args.onScrolled_;
    bar_ = make<ScrollBar>().onUserScrolled([this](float fraction) { setScrollOffset(fraction * content_); });
}

ScrollSlot& ScrollBox::addSlot() { return addSlot(ScrollSlot{}); }

ScrollSlot& ScrollBox::addSlot(ScrollSlot slot) {
    slots_.push_back(std::move(slot));
    return slots_.back();
}

void ScrollBox::removeFront(std::size_t count) {
    slots_.erase(slots_.begin(), slots_.begin() + static_cast<std::ptrdiff_t>(std::min(count, slots_.size())));
}

Widget* ScrollBox::childAt(int index) const {
    if (index < static_cast<int>(slots_.size())) return slots_[static_cast<std::size_t>(index)].widget.get();
    return bar_.get();
}

void ScrollBox::setScrollOffset(float offset) {
    stickToEnd_ = false;
    const float clamped = std::clamp(offset, 0.0f, scrollMax());
    if (clamped == offset_) return;
    offset_ = clamped;
    if (onScrolled_) onScrolled_(offset_);
}

glm::vec2 ScrollBox::computeDesiredSize(float) const {
    glm::vec2 size(0.0f);
    for (const ScrollSlot& slot : slots_) {
        if (!present(slot.widget)) continue;
        const glm::vec2 desired = slot.widget->desiredSize() + slot.padding_.total();
        size.x = std::max(size.x, desired.x);
        size.y += desired.y;
    }
    return size;
}

void ScrollBox::measure(const Geometry& geometry) const {
    float total = 0.0f;
    for (const ScrollSlot& slot : slots_) {
        if (present(slot.widget)) total += slot.widget->desiredSize().y + slot.padding_.top + slot.padding_.bottom;
    }
    content_ = total;
    viewport_ = geometry.size.y;
}

void ScrollBox::arrangeChildren(const Geometry& geometry, ArrangedChildren& out) const {
    measure(geometry);
    auto& self = const_cast<ScrollBox&>(*this);
    if (stickToEnd_) {
        self.offset_ = scrollMax();
        stickToEnd_ = false;
    }
    self.offset_ = std::clamp(offset_, 0.0f, scrollMax());

    const bool scrolling = content_ > viewport_ + 0.5f;
    const float thickness = scrolling ? bar_->thickness() : 0.0f;
    const float width = std::max(0.0f, geometry.size.x - thickness);

    float y = -offset_;
    for (const ScrollSlot& slot : slots_) {
        if (!present(slot.widget)) continue;
        const glm::vec2 desired = slot.widget->desiredSize();
        const Span x = alignHorizontal(slot.hAlign_, width, desired.x, slot.padding_);
        out.push_back({slot.widget, geometry.child({x.offset, y + slot.padding_.top}, {x.size, desired.y})});
        y += desired.y + slot.padding_.top + slot.padding_.bottom;
    }

    bar_->setState(content_ > 0.0f ? offset_ / content_ : 0.0f, content_ > 0.0f ? viewport_ / content_ : 1.0f);
    if (scrolling) out.push_back({bar_, geometry.child({width, 0.0f}, {thickness, geometry.size.y})});
}

int ScrollBox::onPaint(const PaintArgs& args, const Geometry& geometry, ElementList& list, int layer,
                        const PaintStyle& style, bool enabled) const {
    ArrangedChildren arranged;
    arrangeChildren(geometry, arranged);
    int highest = layer;
    list.pushClip(geometry.rect());
    for (const ArrangedWidget& child : arranged) {
        if (child.widget == bar_) continue;
        highest = std::max(highest, child.widget->paint(args, child.geometry, list, layer, style, enabled));
    }
    for (const ArrangedWidget& child : arranged) {
        if (child.widget == bar_) highest = child.widget->paint(args, child.geometry, list, highest + 1, style, enabled);
    }
    list.popClip();
    return highest;
}

Reply ScrollBox::onMouseWheel(const Geometry&, const PointerEvent& event) {
    const float before = offset_;
    setScrollOffset(offset_ - event.wheel.y * WHEEL_STEP);
    return offset_ != before ? Reply::handled() : Reply::unhandled();
}

}
