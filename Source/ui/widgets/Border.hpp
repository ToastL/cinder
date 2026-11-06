#pragma once

#include "ui/core/CompoundWidget.hpp"
#include "ui/core/Args.hpp"

#include <functional>
#include <optional>

namespace cinder::ui {

class Border : public CompoundWidget {
public:
    using MouseHandler = std::function<Reply(const Geometry&, const PointerEvent&)>;

    struct Args : ::cinder::ui::Args<Args, Border> {
        UI_ATTR(Brush, brush)
        UI_ARG(Margin, padding, Margin(2.0f))
        UI_ARG(HAlign, hAlign)
        UI_ARG(VAlign, vAlign)
        UI_ATTR(Color, colorAndOpacity, Color::white())
        UI_ARG(std::optional<Attribute<Color>>, foregroundColor)
        UI_ARG(bool, clip)
        UI_EVENT(MouseHandler, onMouseDown)
        UI_EVENT(MouseHandler, onMouseDoubleClick)
        UI_CONTENT(content)
    };

    void construct(const Args& args);
    void setBrush(Attribute<Brush> brush) { brush_ = std::move(brush); }

    Reply onMouseDown(const Geometry& geometry, const PointerEvent& event) override;
    Reply onMouseDoubleClick(const Geometry& geometry, const PointerEvent& event) override;

protected:
    int onPaint(const PaintArgs& args, const Geometry& geometry, ElementList& list, int layer,
                const PaintStyle& style, bool enabled) const override;

private:
    Attribute<Brush> brush_;
    MouseHandler mouseDown_;
    MouseHandler doubleClick_;
    bool clip_ = false;
};

struct OverlaySlot {
    std::shared_ptr<Widget> widget;
    Margin padding_;
    HAlign hAlign_ = HAlign::Fill;
    VAlign vAlign_ = VAlign::Fill;

    OverlaySlot& padding(Margin value) { padding_ = value; return *this; }
    OverlaySlot& hAlign(HAlign value) { hAlign_ = value; return *this; }
    OverlaySlot& vAlign(VAlign value) { vAlign_ = value; return *this; }
    OverlaySlot& operator[](std::shared_ptr<Widget> content) { widget = std::move(content); return *this; }
};

class Overlay : public Widget {
public:

    static OverlaySlot slot() { return {}; }

    struct Args : ::cinder::ui::Args<Args, Overlay> {
        UI_SLOTS(OverlaySlot, slots)
    };

    void construct(const Args& args) { slots_ = args.slots_; }
    OverlaySlot& addSlot();
    OverlaySlot& addSlot(OverlaySlot slot);
    void removeSlot(const Widget* widget);

    int childCount() const override { return static_cast<int>(slots_.size()); }
    Widget* childAt(int index) const override { return slots_[static_cast<std::size_t>(index)].widget.get(); }
    void arrangeChildren(const Geometry& geometry, ArrangedChildren& out) const override;

protected:
    glm::vec2 computeDesiredSize(float layoutScale) const override;
    int onPaint(const PaintArgs& args, const Geometry& geometry, ElementList& list, int layer,
                const PaintStyle& style, bool enabled) const override;

private:
    std::vector<OverlaySlot> slots_;
};

}
