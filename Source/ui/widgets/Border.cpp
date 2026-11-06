#include "ui/widgets/Border.hpp"

#include "ui/core/ElementList.hpp"

#include <algorithm>

namespace cinder::ui {

void Border::construct(const Args& args) {
    childSlot_.widget = args.content_;
    childSlot_.padding = args.padding_;
    childSlot_.hAlign = args.hAlign_;
    childSlot_.vAlign = args.vAlign_;
    brush_ = args.brush_;
    colorAndOpacity_ = args.colorAndOpacity_;
    foreground_ = args.foregroundColor_;
    mouseDown_ = args.onMouseDown_;
    doubleClick_ = args.onMouseDoubleClick_;
    clip_ = args.clip_;
}

Reply Border::onMouseDown(const Geometry& geometry, const PointerEvent& event) {
    return mouseDown_ ? mouseDown_(geometry, event) : Reply::unhandled();
}

Reply Border::onMouseDoubleClick(const Geometry& geometry, const PointerEvent& event) {
    return doubleClick_ ? doubleClick_(geometry, event) : Reply::unhandled();
}

int Border::onPaint(const PaintArgs& args, const Geometry& geometry, ElementList& list, int layer,
                     const PaintStyle& style, bool enabled) const {
    brush_.get().paint(list, layer, geometry.rect(), style, geometry.scale);
    if (clip_) list.pushClip(geometry.rect());
    const int result = paintChildren(args, geometry, list, layer + 1, childStyle(style), enabled);
    if (clip_) list.popClip();
    return result;
}

OverlaySlot& Overlay::addSlot() { return addSlot(OverlaySlot{}); }

OverlaySlot& Overlay::addSlot(OverlaySlot slot) {
    slots_.push_back(std::move(slot));
    return slots_.back();
}

void Overlay::removeSlot(const Widget* widget) {
    std::erase_if(slots_, [widget](const OverlaySlot& slot) { return slot.widget.get() == widget; });
}

glm::vec2 Overlay::computeDesiredSize(float) const {
    glm::vec2 size(0.0f);
    for (const OverlaySlot& slot : slots_) {
        if (!slot.widget || !takesSpace(slot.widget->visibility())) continue;
        size = glm::max(size, slot.widget->desiredSize() + slot.padding_.total());
    }
    return size;
}

void Overlay::arrangeChildren(const Geometry& geometry, ArrangedChildren& out) const {
    for (const OverlaySlot& slot : slots_) {
        if (!slot.widget || !takesSpace(slot.widget->visibility())) continue;
        const glm::vec2 desired = slot.widget->desiredSize();
        const Span x = alignHorizontal(slot.hAlign_, geometry.size.x, desired.x, slot.padding_);
        const Span y = alignVertical(slot.vAlign_, geometry.size.y, desired.y, slot.padding_);
        out.push_back({slot.widget, geometry.child({x.offset, y.offset}, {x.size, y.size})});
    }
}

int Overlay::onPaint(const PaintArgs& args, const Geometry& geometry, ElementList& list, int layer,
                      const PaintStyle& style, bool enabled) const {
    ArrangedChildren arranged;
    arrangeChildren(geometry, arranged);
    int highest = layer;
    for (const ArrangedWidget& child : arranged) {
        highest = child.widget->paint(args, child.geometry, list, highest + 1, style, enabled);
    }
    return highest;
}

}
