#pragma once

#include "ui/core/Args.hpp"
#include "ui/core/Layout.hpp"
#include "ui/core/Widget.hpp"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace cinder::ui {

class ScrollBar : public LeafWidget {
public:
    struct Args : ::cinder::ui::Args<Args, ScrollBar> {
        UI_ARG(Orientation, orientation, Orientation::Vertical)
        UI_EVENT(std::function<void(float)>, onUserScrolled)
        UI_ARG(std::string, style, "ScrollBar")
    };

    void construct(const Args& args);
    void setState(float offset, float thumb);
    bool needed() const { return thumb_ < 1.0f; }
    float thickness() const;
    bool dragging() const { return dragging_; }

    Reply onMouseDown(const Geometry& geometry, const PointerEvent& event) override;
    Reply onMouseMove(const Geometry& geometry, const PointerEvent& event) override;
    Reply onMouseUp(const Geometry& geometry, const PointerEvent& event) override;
    void onMouseCaptureLost() override { dragging_ = false; }

protected:
    glm::vec2 computeDesiredSize(float layoutScale) const override;
    int onPaint(const PaintArgs& args, const Geometry& geometry, ElementList& list, int layer,
                const PaintStyle& style, bool enabled) const override;

private:
    Span thumbSpan(const Geometry& geometry) const;
    float along(glm::vec2 value) const { return orientation_ == Orientation::Vertical ? value.y : value.x; }

    Orientation orientation_ = Orientation::Vertical;
    std::function<void(float)> onScrolled_;
    std::string style_;
    float offset_ = 0.0f;
    float thumb_ = 1.0f;
    float grab_ = 0.0f;
    bool dragging_ = false;
};

struct ScrollSlot {
    std::shared_ptr<Widget> widget;
    Margin padding_;
    HAlign hAlign_ = HAlign::Fill;

    ScrollSlot& padding(Margin value) { padding_ = value; return *this; }
    ScrollSlot& hAlign(HAlign value) { hAlign_ = value; return *this; }
    ScrollSlot& operator[](std::shared_ptr<Widget> content) { widget = std::move(content); return *this; }
};

class ScrollBox : public Widget {
public:
    static constexpr float WHEEL_STEP = 20.0f;
    static ScrollSlot slot() { return {}; }

    struct Args : ::cinder::ui::Args<Args, ScrollBox> {
        UI_SLOTS(ScrollSlot, slots)
        UI_EVENT(std::function<void(float)>, onScrolled)
    };

    void construct(const Args& args);

    ScrollSlot& addSlot();
    ScrollSlot& addSlot(ScrollSlot slot);
    void clearChildren() { slots_.clear(); }
    void removeFront(std::size_t count);
    std::size_t slotCount() const { return slots_.size(); }

    float scrollOffset() const { return offset_; }
    void setScrollOffset(float offset);
    void scrollToEnd() { stickToEnd_ = true; }
    float scrollMax() const { return std::max(0.0f, content_ - viewport_); }
    bool atEnd() const { return offset_ >= scrollMax() - 0.5f; }

    int childCount() const override { return static_cast<int>(slots_.size()) + 1; }
    Widget* childAt(int index) const override;
    void arrangeChildren(const Geometry& geometry, ArrangedChildren& out) const override;

    Reply onMouseWheel(const Geometry& geometry, const PointerEvent& event) override;

protected:
    glm::vec2 computeDesiredSize(float layoutScale) const override;
    int onPaint(const PaintArgs& args, const Geometry& geometry, ElementList& list, int layer,
                const PaintStyle& style, bool enabled) const override;

private:
    void measure(const Geometry& geometry) const;

    std::vector<ScrollSlot> slots_;
    std::shared_ptr<ScrollBar> bar_;
    std::function<void(float)> onScrolled_;
    float offset_ = 0.0f;
    mutable float content_ = 0.0f;
    mutable float viewport_ = 0.0f;
    mutable bool stickToEnd_ = false;
};

}
