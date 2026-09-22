#pragma once

#include "ui/core/CompoundWidget.hpp"
#include "ui/core/Args.hpp"
#include "ui/core/Delegates.hpp"

#include <optional>
#include <string>

namespace cinder::ui {

class Button : public CompoundWidget {
public:
    struct Args : ::cinder::ui::Args<Args, Button> {
        UI_ATTR(std::string, text)
        UI_EVENT(OnClicked, onClicked)
        UI_ARG(std::string, buttonStyle, "Button")
        UI_ARG(std::optional<Margin>, contentPadding)
        UI_ARG(HAlign, hAlign, HAlign::Center)
        UI_ARG(VAlign, vAlign, VAlign::Center)
        UI_ARG(bool, isFocusable)
        UI_CONTENT(content)
    };

    void construct(const Args& args);

    bool isPressed() const { return pressed_; }

    Reply onMouseDown(const Geometry& geometry, const PointerEvent& event) override;
    Reply onMouseDoubleClick(const Geometry& geometry, const PointerEvent& event) override;
    Reply onMouseUp(const Geometry& geometry, const PointerEvent& event) override;
    Reply onKeyDown(const Geometry& geometry, const KeyEvent& event) override;
    void onMouseCaptureLost() override { pressed_ = false; }
    bool supportsKeyboardFocus() const override { return focusable_; }

protected:
    glm::vec2 computeDesiredSize(float layoutScale) const override;
    void arrangeChildren(const Geometry& geometry, ArrangedChildren& out) const override;
    int onPaint(const PaintArgs& args, const Geometry& geometry, ElementList& list, int layer,
                const PaintStyle& style, bool enabled) const override;

private:
    Margin padding() const;
    Reply click();

    OnClicked onClicked_;
    std::string style_;
    std::optional<Margin> padding_;
    bool focusable_ = false;
    bool pressed_ = false;
};

class CheckBox : public CompoundWidget {
public:
    struct Args : ::cinder::ui::Args<Args, CheckBox> {
        UI_ATTR(bool, isChecked)
        UI_EVENT(OnBoolChanged, onCheckStateChanged)
        UI_ARG(std::string, style, "CheckBox")
        UI_CONTENT(content)
    };

    void construct(const Args& args);

    bool isChecked() const { return checked_.get(); }

    Reply onMouseDown(const Geometry& geometry, const PointerEvent& event) override;
    Reply onMouseUp(const Geometry& geometry, const PointerEvent& event) override;
    void onMouseCaptureLost() override { pressed_ = false; }

protected:
    glm::vec2 computeDesiredSize(float layoutScale) const override;
    void arrangeChildren(const Geometry& geometry, ArrangedChildren& out) const override;
    int onPaint(const PaintArgs& args, const Geometry& geometry, ElementList& list, int layer,
                const PaintStyle& style, bool enabled) const override;

private:
    Attribute<bool> checked_;
    OnBoolChanged onChanged_;
    std::string style_;
    bool pressed_ = false;
};

}
