#include "UI/Widgets/DropTarget.hpp"

#include "UI/Core/ElementList.hpp"
#include "UI/Framework/Application.hpp"

namespace cinder::UI {

void DropTarget::construct(const Args& args) {
    childSlot_.widget = args.content_;
    canAccept_ = args.onCanAccept_;
    accept_ = args.onDrop_;
}

void DropTarget::onDragLeave(const DragDropEvent&) { hovered_ = false; }

Reply DropTarget::onDragOver(const Geometry&, const DragDropEvent& event) {
    hovered_ = canAccept_ && canAccept_(event);
    return hovered_ ? Reply::handled() : Reply::unhandled();
}

Reply DropTarget::onDrop(const Geometry&, const DragDropEvent& event) {
    hovered_ = false;
    if (!canAccept_ || !accept_ || !canAccept_(event)) return Reply::unhandled();
    accept_(event);
    return Reply::handled();
}

int DropTarget::onPaint(const PaintArgs& args, const Geometry& geometry, ElementList& list, int layer,
                        const PaintStyle& style, bool enabled) const {
    const int highest = CompoundWidget::onPaint(args, geometry, list, layer, style, enabled);
    if (!hovered_) return highest;
    const Color mark = style.apply(Application::get().theme().color("Color.Primary"));
    list.box(highest + 1, geometry.rect(), BoxStyle{Color::transparent(), mark, 1.5f * geometry.scale, glm::vec4(3.0f)});
    return highest + 1;
}

}
