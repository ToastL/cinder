#pragma once

#include "platform/Cursor.hpp"
#include "ui/core/Attribute.hpp"
#include "ui/core/DragDrop.hpp"
#include "ui/core/Events.hpp"
#include "ui/core/Geometry.hpp"
#include "ui/core/Reply.hpp"
#include "ui/core/Style.hpp"

#include <glm/vec2.hpp>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace cinder::ui {

class ElementList;
class HitTester;
class Widget;

enum class Visibility : std::uint8_t { Visible, Collapsed, Hidden, NoHitTest, NoHitTestSelf };

inline bool takesSpace(Visibility visibility) { return visibility != Visibility::Collapsed; }
inline bool isShown(Visibility visibility) {
    return visibility != Visibility::Collapsed && visibility != Visibility::Hidden;
}
inline bool hitsSelf(Visibility visibility) { return visibility == Visibility::Visible; }
inline bool hitsChildren(Visibility visibility) {
    return visibility == Visibility::Visible || visibility == Visibility::NoHitTestSelf;
}

struct ArrangedWidget {
    std::shared_ptr<Widget> widget;
    Geometry geometry;
};

using ArrangedChildren = std::vector<ArrangedWidget>;

struct PaintArgs {
    HitTester* grid = nullptr;
    double time = 0.0;
    float deltaTime = 0.0f;
};

class Widget : public std::enable_shared_from_this<Widget> {
public:
    Widget() = default;
    virtual ~Widget() = default;

    Widget(const Widget&) = delete;
    Widget& operator=(const Widget&) = delete;

    void prepass(float layoutScale);
    glm::vec2 desiredSize() const { return desired_; }

    virtual int childCount() const { return 0; }
    virtual Widget* childAt(int index) const { return nullptr; }
    virtual void arrangeChildren(const Geometry& geometry, ArrangedChildren& out) const {}

    int paint(const PaintArgs& args, const Geometry& geometry, ElementList& list, int layer, const PaintStyle& style,
              bool parentEnabled) const;

    virtual Reply onMouseDown(const Geometry& geometry, const PointerEvent& event);
    virtual Reply onMouseUp(const Geometry& geometry, const PointerEvent& event);
    virtual Reply onMouseMove(const Geometry& geometry, const PointerEvent& event);
    virtual Reply onMouseDoubleClick(const Geometry& geometry, const PointerEvent& event);
    virtual Reply onMouseWheel(const Geometry& geometry, const PointerEvent& event);
    virtual void onMouseEnter(const Geometry& geometry, const PointerEvent& event) {}
    virtual void onMouseLeave(const PointerEvent& event) {}
    virtual Reply onDragDetected(const Geometry& geometry, const PointerEvent& event);
    virtual void onDragEnter(const Geometry& geometry, const DragDropEvent& event) {}
    virtual void onDragLeave(const DragDropEvent& event) {}
    virtual Reply onDragOver(const Geometry& geometry, const DragDropEvent& event);
    virtual Reply onDrop(const Geometry& geometry, const DragDropEvent& event);
    virtual void onMouseCaptureLost() {}
    virtual Reply onKeyDown(const Geometry& geometry, const KeyEvent& event);
    virtual Reply onKeyUp(const Geometry& geometry, const KeyEvent& event);
    virtual Reply onKeyChar(const Geometry& geometry, const CharEvent& event);
    virtual Reply onFocusReceived(const Geometry& geometry, const FocusEvent& event);
    virtual void onFocusLost(const FocusEvent& event) {}
    virtual std::optional<cinder::platform::CursorShape> onCursorQuery(const Geometry& geometry,
                                                                       const PointerEvent& event) const;
    virtual bool supportsKeyboardFocus() const { return false; }
    virtual bool isEditingText() const { return false; }
    virtual void tick(const Geometry& geometry, double time, float deltaTime) {}

    Visibility visibility() const { return visibility_.get(); }
    void setVisibility(Attribute<Visibility> visibility) { visibility_ = std::move(visibility); }
    bool isEnabled() const { return enabled_.get(); }
    void setEnabled(Attribute<bool> enabled) { enabled_ = std::move(enabled); }
    std::string toolTipText() const { return toolTip_.get(); }
    void setToolTipText(Attribute<std::string> text) { toolTip_ = std::move(text); }
    std::optional<cinder::platform::CursorShape> cursor() const { return cursor_; }
    void setCursor(std::optional<cinder::platform::CursorShape> cursor) { cursor_ = cursor; }

    bool isHovered() const { return hovered_; }
    bool hasFocus() const { return focused_; }
    bool hasMouseCapture() const { return captured_; }

protected:
    virtual glm::vec2 computeDesiredSize(float layoutScale) const = 0;
    virtual int onPaint(const PaintArgs& args, const Geometry& geometry, ElementList& list, int layer,
                        const PaintStyle& style, bool enabled) const = 0;

    int paintChildren(const PaintArgs& args, const Geometry& geometry, ElementList& list, int layer,
                      const PaintStyle& style, bool enabled) const;

private:
    friend class Application;

    glm::vec2 desired_{0.0f};
    Attribute<Visibility> visibility_{Visibility::Visible};
    Attribute<bool> enabled_{true};
    Attribute<std::string> toolTip_;
    std::optional<cinder::platform::CursorShape> cursor_;
    bool hovered_ = false;
    bool focused_ = false;
    bool captured_ = false;
};

class LeafWidget : public Widget {};

}
