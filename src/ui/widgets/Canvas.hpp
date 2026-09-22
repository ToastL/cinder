#pragma once

#include "ui/core/Args.hpp"
#include "ui/core/Layout.hpp"
#include "ui/core/Widget.hpp"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace cinder::ui {

struct CanvasSlot {
    std::shared_ptr<Widget> widget;
    glm::vec2 anchorMin_{0.0f};
    glm::vec2 anchorMax_{0.0f};
    Margin offset_{0.0f, 0.0f, 100.0f, 30.0f};
    glm::vec2 alignment_{0.0f};
    bool autoSize_ = false;
    int zOrder_ = 0;

    CanvasSlot& anchors(glm::vec2 minimum, glm::vec2 maximum) {
        anchorMin_ = minimum;
        anchorMax_ = maximum;
        return *this;
    }
    CanvasSlot& anchors(glm::vec2 point) { return anchors(point, point); }
    CanvasSlot& offset(Margin value) { offset_ = value; return *this; }
    CanvasSlot& alignment(glm::vec2 value) { alignment_ = value; return *this; }
    CanvasSlot& autoSize(bool value) { autoSize_ = value; return *this; }
    CanvasSlot& zOrder(int value) { zOrder_ = value; return *this; }
    CanvasSlot& operator[](std::shared_ptr<Widget> content) { widget = std::move(content); return *this; }
};

class Canvas : public Widget {
public:
    static CanvasSlot slot() { return {}; }

    struct Args : ::cinder::ui::Args<Args, Canvas> {
        UI_SLOTS(CanvasSlot, slots)
    };

    void construct(const Args& args) { slots_ = args.slots_; }
    CanvasSlot& addSlot(CanvasSlot slot);

    int childCount() const override { return static_cast<int>(slots_.size()); }
    Widget* childAt(int index) const override { return slots_[static_cast<std::size_t>(index)].widget.get(); }
    void arrangeChildren(const Geometry& geometry, ArrangedChildren& out) const override;

protected:
    glm::vec2 computeDesiredSize(float layoutScale) const override;
    int onPaint(const PaintArgs& args, const Geometry& geometry, ElementList& list, int layer,
                const PaintStyle& style, bool enabled) const override;

private:
    std::vector<CanvasSlot> slots_;
};

}
