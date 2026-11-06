#pragma once

#include "ui/core/Args.hpp"
#include "ui/core/Layout.hpp"
#include "ui/core/Widget.hpp"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace cinder::ui {

struct SplitterSlot {
    std::shared_ptr<Widget> widget;
    Attribute<float> value_{1.0f};
    std::function<void(float)> onSlotResized_;

    SplitterSlot& value(Attribute<float> size) { value_ = std::move(size); return *this; }
    SplitterSlot& onSlotResized(std::function<void(float)> handler) {
        onSlotResized_ = std::move(handler);
        return *this;
    }
    SplitterSlot& operator[](std::shared_ptr<Widget> content) { widget = std::move(content); return *this; }
};

class Splitter : public Widget {
public:
    static constexpr float MIN_SLOT = 20.0f;
    static SplitterSlot slot() { return {}; }

    struct Args : ::cinder::ui::Args<Args, Splitter> {
        UI_ARG(Orientation, orientation, Orientation::Horizontal)
        UI_ARG(std::string, style, "Splitter")
        UI_SLOTS(SplitterSlot, slots)
        UI_EVENT(std::function<void()>, onFinishedResizing)
    };

    void construct(const Args& args);
    SplitterSlot& addSlot(SplitterSlot slot);
    std::size_t slotCount() const { return slots_.size(); }
    float slotValue(std::size_t index) const { return slots_[index].value_.get(); }

    int childCount() const override { return static_cast<int>(slots_.size()); }
    Widget* childAt(int index) const override { return slots_[static_cast<std::size_t>(index)].widget.get(); }
    void arrangeChildren(const Geometry& geometry, ArrangedChildren& out) const override;

    Reply onMouseDown(const Geometry& geometry, const PointerEvent& event) override;
    Reply onMouseMove(const Geometry& geometry, const PointerEvent& event) override;
    Reply onMouseUp(const Geometry& geometry, const PointerEvent& event) override;
    void onMouseCaptureLost() override { dragged_ = -1; }
    std::optional<cinder::platform::CursorShape> onCursorQuery(const Geometry& geometry,
                                                               const PointerEvent& event) const override;

protected:
    glm::vec2 computeDesiredSize(float layoutScale) const override;
    int onPaint(const PaintArgs& args, const Geometry& geometry, ElementList& list, int layer,
                const PaintStyle& style, bool enabled) const override;

private:
    float handleSize() const;
    std::vector<Span> spans(const Geometry& geometry) const;
    int handleAt(const Geometry& geometry, glm::vec2 point) const;
    void setValue(std::size_t index, float value);

    Orientation orientation_ = Orientation::Horizontal;
    std::string style_;
    std::vector<SplitterSlot> slots_;
    std::function<void()> onFinished_;
    int dragged_ = -1;
    int hovered_ = -1;
};

}
