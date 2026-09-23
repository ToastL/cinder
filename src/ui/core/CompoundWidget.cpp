#include "ui/core/CompoundWidget.hpp"

namespace cinder::ui {

glm::vec2 CompoundWidget::computeDesiredSize(float) const {
    const glm::vec2 content = childSlot_.widget && takesSpace(childSlot_.widget->visibility())
            ? childSlot_.widget->desiredSize()
            : glm::vec2(0.0f);
    return content + childSlot_.padding.total();
}

void CompoundWidget::arrangeChildren(const Geometry& geometry, ArrangedChildren& out) const {
    if (!childSlot_.widget || !takesSpace(childSlot_.widget->visibility())) return;
    const glm::vec2 desired = childSlot_.widget->desiredSize();
    const Span x = alignHorizontal(childSlot_.hAlign, geometry.size.x, desired.x, childSlot_.padding);
    const Span y = alignVertical(childSlot_.vAlign, geometry.size.y, desired.y, childSlot_.padding);
    out.push_back({childSlot_.widget, geometry.child({x.offset, y.offset}, {x.size, y.size})});
}

PaintStyle CompoundWidget::childStyle(const PaintStyle& style) const {
    PaintStyle child = style;
    child.tint = style.tint * colorAndOpacity_.get();
    if (foreground_) child.foreground = foreground_->get();
    return child;
}

int CompoundWidget::onPaint(const PaintArgs& args, const Geometry& geometry, ElementList& list, int layer,
                            const PaintStyle& style, bool enabled) const {
    return paintChildren(args, geometry, list, layer, childStyle(style), enabled);
}

}
