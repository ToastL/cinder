#include "ui/widgets/BoxPanel.hpp"

#include <algorithm>

namespace cinder::ui {

namespace {

bool present(const BoxSlot& slot) { return slot.widget && takesSpace(slot.widget->visibility()); }

float along(glm::vec2 value, Orientation orientation) {
    return orientation == Orientation::Horizontal ? value.x : value.y;
}

float across(glm::vec2 value, Orientation orientation) {
    return orientation == Orientation::Horizontal ? value.y : value.x;
}

float clampMax(float value, float max) { return max > 0.0f ? std::min(value, max) : value; }

}

BoxSlot& BoxPanel::addSlot() { return addSlot(BoxSlot{}); }

BoxSlot& BoxPanel::addSlot(BoxSlot slot) {
    slots_.push_back(std::move(slot));
    return slots_.back();
}

glm::vec2 BoxPanel::computeDesiredSize(float) const {
    float length = 0.0f;
    float thickness = 0.0f;
    for (const BoxSlot& slot : slots_) {
        if (!present(slot)) continue;
        const glm::vec2 padding = slot.padding_.total();
        const glm::vec2 desired = slot.widget->desiredSize();
        length += clampMax(along(desired, orientation_), slot.max_) + along(padding, orientation_);
        thickness = std::max(thickness, across(desired, orientation_) + across(padding, orientation_));
    }
    return orientation_ == Orientation::Horizontal ? glm::vec2(length, thickness) : glm::vec2(thickness, length);
}

void BoxPanel::arrangeChildren(const Geometry& geometry, ArrangedChildren& out) const {
    const float allotted = along(geometry.size, orientation_);
    const float cross = across(geometry.size, orientation_);

    float fixed = 0.0f;
    float fillTotal = 0.0f;
    for (const BoxSlot& slot : slots_) {
        if (!present(slot)) continue;
        fixed += along(slot.padding_.total(), orientation_);
        if (slot.auto_) fixed += clampMax(along(slot.widget->desiredSize(), orientation_), slot.max_);
        else fillTotal += slot.fill_;
    }
    const float remaining = std::max(0.0f, allotted - fixed);

    float offset = 0.0f;
    for (const BoxSlot& slot : slots_) {
        if (!present(slot)) continue;
        const glm::vec2 desired = slot.widget->desiredSize();
        float size = slot.auto_ ? along(desired, orientation_)
                                : (fillTotal > 0.0f ? remaining * slot.fill_ / fillTotal : 0.0f);
        size = clampMax(size, slot.max_);

        if (orientation_ == Orientation::Horizontal) {
            const float span = size + slot.padding_.left + slot.padding_.right;
            const Span x = alignHorizontal(slot.hAlign_, span, desired.x, slot.padding_);
            const Span y = alignVertical(slot.vAlign_, cross, desired.y, slot.padding_);
            out.push_back({slot.widget, geometry.child({offset + x.offset, y.offset}, {x.size, y.size})});
            offset += span;
        } else {
            const float span = size + slot.padding_.top + slot.padding_.bottom;
            const Span x = alignHorizontal(slot.hAlign_, cross, desired.x, slot.padding_);
            const Span y = alignVertical(slot.vAlign_, span, desired.y, slot.padding_);
            out.push_back({slot.widget, geometry.child({x.offset, offset + y.offset}, {x.size, y.size})});
            offset += span;
        }
    }
}

int BoxPanel::onPaint(const PaintArgs& args, const Geometry& geometry, ElementList& list, int layer,
                       const PaintStyle& style, bool enabled) const {
    return paintChildren(args, geometry, list, layer, style, enabled);
}

}
