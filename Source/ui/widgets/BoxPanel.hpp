#pragma once

#include "ui/core/Args.hpp"
#include "ui/core/Layout.hpp"
#include "ui/core/Widget.hpp"

#include <memory>
#include <vector>

namespace cinder::ui {

struct BoxSlot {
    std::shared_ptr<Widget> widget;
    Margin padding_;
    HAlign hAlign_ = HAlign::Fill;
    VAlign vAlign_ = VAlign::Fill;
    bool auto_ = false;
    float fill_ = 1.0f;
    float max_ = 0.0f;

    BoxSlot& autoWidth() { auto_ = true; return *this; }
    BoxSlot& autoHeight() { auto_ = true; return *this; }
    BoxSlot& fill(float value) { auto_ = false; fill_ = value; return *this; }
    BoxSlot& fillWidth(float value) { return fill(value); }
    BoxSlot& fillHeight(float value) { return fillWidth(value); }
    BoxSlot& maxWidth(float value) { max_ = value; return *this; }
    BoxSlot& maxHeight(float value) { return maxWidth(value); }
    BoxSlot& padding(Margin value) { padding_ = value; return *this; }
    BoxSlot& hAlign(HAlign value) { hAlign_ = value; return *this; }
    BoxSlot& vAlign(VAlign value) { vAlign_ = value; return *this; }
    BoxSlot& operator[](std::shared_ptr<Widget> content) { widget = std::move(content); return *this; }
};

class BoxPanel : public Widget {
public:

    static BoxSlot slot() { return {}; }

    explicit BoxPanel(Orientation orientation) : orientation_(orientation) {}

    BoxSlot& addSlot();
    BoxSlot& addSlot(BoxSlot slot);
    void clearChildren() { slots_.clear(); }
    std::size_t slotCount() const { return slots_.size(); }
    BoxSlot& slotAt(std::size_t index) { return slots_[index]; }

    int childCount() const override { return static_cast<int>(slots_.size()); }
    Widget* childAt(int index) const override { return slots_[static_cast<std::size_t>(index)].widget.get(); }
    void arrangeChildren(const Geometry& geometry, ArrangedChildren& out) const override;

protected:
    glm::vec2 computeDesiredSize(float layoutScale) const override;
    int onPaint(const PaintArgs& args, const Geometry& geometry, ElementList& list, int layer,
                const PaintStyle& style, bool enabled) const override;

    Orientation orientation_;
    std::vector<BoxSlot> slots_;
};

class HorizontalBox : public BoxPanel {
public:
    struct Args : ::cinder::ui::Args<Args, HorizontalBox> {
        UI_SLOTS(BoxSlot, slots)
    };

    HorizontalBox() : BoxPanel(Orientation::Horizontal) {}
    void construct(const Args& args) { slots_ = args.slots_; }
};

class VerticalBox : public BoxPanel {
public:
    struct Args : ::cinder::ui::Args<Args, VerticalBox> {
        UI_SLOTS(BoxSlot, slots)
    };

    VerticalBox() : BoxPanel(Orientation::Vertical) {}
    void construct(const Args& args) { slots_ = args.slots_; }
};

}
