#include "ui/core/Widget.hpp"

#include "ui/core/ElementList.hpp"
#include "ui/core/HitTester.hpp"

#include <algorithm>

namespace cinder::ui {

void Widget::prepass(float layoutScale) {
    if (visibility() == Visibility::Collapsed) {
        desired_ = glm::vec2(0.0f);
        return;
    }
    for (int i = 0; i < childCount(); ++i) {
        if (Widget* child = childAt(i)) child->prepass(layoutScale);
    }
    desired_ = computeDesiredSize(layoutScale);
}

int Widget::paint(const PaintArgs& args, const Geometry& geometry, ElementList& list, int layer,
                  const PaintStyle& style, bool parentEnabled) const {
    const Visibility shown = visibility();
    if (!isShown(shown)) return layer;
    if (geometry.rect().intersect(list.clip()).empty()) return layer;

    auto& self = const_cast<Widget&>(*this);
    self.tick(geometry, args.time, args.deltaTime);

    const bool enabled = parentEnabled && isEnabled();
    if (args.grid != nullptr) args.grid->push(self, geometry, list.clip(), layer, shown, enabled);
    const int result = onPaint(args, geometry, list, layer, style, enabled);
    if (args.grid != nullptr) args.grid->pop();
    return result;
}

int Widget::paintChildren(const PaintArgs& args, const Geometry& geometry, ElementList& list, int layer,
                          const PaintStyle& style, bool enabled) const {
    ArrangedChildren arranged;
    arrangeChildren(geometry, arranged);
    int highest = layer;
    for (const ArrangedWidget& child : arranged) {
        highest = std::max(highest, child.widget->paint(args, child.geometry, list, layer, style, enabled));
    }
    return highest;
}

Reply Widget::onMouseDown(const Geometry&, const PointerEvent&) { return Reply::unhandled(); }
Reply Widget::onMouseUp(const Geometry&, const PointerEvent&) { return Reply::unhandled(); }
Reply Widget::onMouseMove(const Geometry&, const PointerEvent&) { return Reply::unhandled(); }
Reply Widget::onMouseDoubleClick(const Geometry&, const PointerEvent&) { return Reply::unhandled(); }
Reply Widget::onMouseWheel(const Geometry&, const PointerEvent&) { return Reply::unhandled(); }
Reply Widget::onDragDetected(const Geometry&, const PointerEvent&) { return Reply::unhandled(); }
Reply Widget::onDragOver(const Geometry&, const DragDropEvent&) { return Reply::unhandled(); }
Reply Widget::onDrop(const Geometry&, const DragDropEvent&) { return Reply::unhandled(); }
Reply Widget::onKeyDown(const Geometry&, const KeyEvent&) { return Reply::unhandled(); }
Reply Widget::onKeyUp(const Geometry&, const KeyEvent&) { return Reply::unhandled(); }
Reply Widget::onKeyChar(const Geometry&, const CharEvent&) { return Reply::unhandled(); }
Reply Widget::onFocusReceived(const Geometry&, const FocusEvent&) { return Reply::unhandled(); }

std::optional<cinder::platform::CursorShape> Widget::onCursorQuery(const Geometry&, const PointerEvent&) const {
    return std::nullopt;
}

}
