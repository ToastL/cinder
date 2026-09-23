#include "ui/widgets/Canvas.hpp"

#include "ui/core/ElementList.hpp"
#include "ui/framework/Application.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace cinder::ui {

namespace {

bool present(const std::shared_ptr<Widget>& widget) { return widget && takesSpace(widget->visibility()); }

}

CanvasSlot& Canvas::addSlot(CanvasSlot slot) {
    slots_.push_back(std::move(slot));
    return slots_.back();
}

glm::vec2 Canvas::computeDesiredSize(float) const {
    glm::vec2 size(0.0f);
    for (const CanvasSlot& slot : slots_) {
        if (!present(slot.widget)) continue;
        const glm::vec2 desired = slot.widget->desiredSize();
        const glm::vec2 extent = slot.autoSize_ ? desired : glm::vec2(slot.offset_.right, slot.offset_.bottom);
        size = glm::max(size, glm::vec2(slot.offset_.left, slot.offset_.top) + extent);
    }
    return size;
}

void Canvas::arrangeChildren(const Geometry& geometry, ArrangedChildren& out) const {
    std::vector<std::size_t> order(slots_.size());
    std::iota(order.begin(), order.end(), std::size_t{0});
    std::stable_sort(order.begin(), order.end(),
                     [this](std::size_t a, std::size_t b) { return slots_[a].zOrder_ < slots_[b].zOrder_; });

    for (const std::size_t index : order) {
        const CanvasSlot& slot = slots_[index];
        if (!present(slot.widget)) continue;
        const glm::vec2 desired = slot.widget->desiredSize();
        const glm::vec2 low = geometry.size * slot.anchorMin_;
        const glm::vec2 high = geometry.size * slot.anchorMax_;

        const auto axis = [&](int i, float start, float end, float lead, float trail, float want) {
            if (slot.anchorMin_[i] == slot.anchorMax_[i]) {
                const float size = slot.autoSize_ ? want : trail;
                return Span{start + lead - slot.alignment_[i] * size, size};
            }
            const float position = start + lead;
            return Span{position, std::max(0.0f, end - trail - position)};
        };
        const Span x = axis(0, low.x, high.x, slot.offset_.left, slot.offset_.right, desired.x);
        const Span y = axis(1, low.y, high.y, slot.offset_.top, slot.offset_.bottom, desired.y);
        out.push_back({slot.widget, geometry.child({x.offset, y.offset}, {x.size, y.size})});
    }
}

int Canvas::onPaint(const PaintArgs& args, const Geometry& geometry, ElementList& list, int layer,
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
