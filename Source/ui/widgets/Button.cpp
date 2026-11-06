#include "ui/widgets/Button.hpp"

#include "ui/core/ElementList.hpp"
#include "ui/framework/Application.hpp"
#include "ui/widgets/Label.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace cinder::ui {

namespace keys = cinder::platform::keys;

void Button::construct(const Args& args) {
    onClicked_ = args.onClicked_;
    style_ = args.buttonStyle_;
    padding_ = args.contentPadding_;
    focusable_ = args.isFocusable_;
    childSlot_.hAlign = args.hAlign_;
    childSlot_.vAlign = args.vAlign_;
    childSlot_.widget = args.content_ ? args.content_ : (make<Label>().text(args.text_));
}

Margin Button::padding() const {
    return padding_.value_or(Application::get().theme().get<ButtonStyle>(style_.get()).padding);
}

glm::vec2 Button::computeDesiredSize(float) const {
    const glm::vec2 content = childSlot_.widget && takesSpace(childSlot_.widget->visibility())
            ? childSlot_.widget->desiredSize()
            : glm::vec2(0.0f);
    return content + padding().total();
}

void Button::arrangeChildren(const Geometry& geometry, ArrangedChildren& out) const {
    if (!childSlot_.widget || !takesSpace(childSlot_.widget->visibility())) return;
    const Margin inset = padding();
    const glm::vec2 desired = childSlot_.widget->desiredSize();
    const Span x = alignHorizontal(childSlot_.hAlign, geometry.size.x, desired.x, inset);
    const Span y = alignVertical(childSlot_.vAlign, geometry.size.y, desired.y, inset);
    out.push_back({childSlot_.widget, geometry.child({x.offset, y.offset}, {x.size, y.size})});
}

Reply Button::click() {
    return onClicked_ ? onClicked_() : Reply::handled();
}

Reply Button::onMouseDown(const Geometry&, const PointerEvent& event) {
    if (event.button != cinder::platform::buttons::LEFT) return Reply::unhandled();
    pressed_ = true;
    return Reply::handled().captureMouse(shared_from_this());
}

Reply Button::onMouseDoubleClick(const Geometry& geometry, const PointerEvent& event) {
    return onMouseDown(geometry, event);
}

Reply Button::onMouseUp(const Geometry& geometry, const PointerEvent& event) {
    if (event.button != cinder::platform::buttons::LEFT) return Reply::unhandled();
    const bool clicked = pressed_ && geometry.contains(event.position);
    pressed_ = false;
    Reply reply = clicked ? click() : Reply::handled();
    if (!reply.isHandled()) reply = Reply::handled();
    return reply.releaseMouseCapture();
}

Reply Button::onKeyDown(const Geometry&, const KeyEvent& event) {
    if (event.key == keys::ENTER || event.key == keys::KEYPAD_ENTER || event.key == keys::SPACE) return click();
    return Reply::unhandled();
}

int Button::onPaint(const PaintArgs& args, const Geometry& geometry, ElementList& list, int layer,
                     const PaintStyle& style, bool enabled) const {
    const ButtonStyle& look = Application::get().theme().get<ButtonStyle>(style_.get());
    const bool down = pressed_ && isHovered();
    const Brush& brush = !enabled ? look.disabled : (down ? look.pressed : (isHovered() ? look.hovered : look.normal));
    brush.paint(list, layer, geometry.rect(), style, geometry.scale);

    PaintStyle content = childStyle(style);
    content.foreground = enabled ? look.foreground : look.disabledForeground;
    return paintChildren(args, geometry, list, layer + 1, content, enabled);
}

void CheckBox::construct(const Args& args) {
    checked_ = args.isChecked_;
    onChanged_ = args.onCheckStateChanged_;
    style_ = args.style_;
    childSlot_.widget = args.content_;
    childSlot_.vAlign = VAlign::Center;
}

glm::vec2 CheckBox::computeDesiredSize(float) const {
    const CheckBoxStyle& look = Application::get().theme().get<CheckBoxStyle>(style_);
    if (!childSlot_.widget || !takesSpace(childSlot_.widget->visibility())) return glm::vec2(look.size);
    const glm::vec2 content = childSlot_.widget->desiredSize();
    return {look.size + look.spacing + content.x, std::max(look.size, content.y)};
}

void CheckBox::arrangeChildren(const Geometry& geometry, ArrangedChildren& out) const {
    if (!childSlot_.widget || !takesSpace(childSlot_.widget->visibility())) return;
    const CheckBoxStyle& look = Application::get().theme().get<CheckBoxStyle>(style_);
    const float left = look.size + look.spacing;
    const glm::vec2 desired = childSlot_.widget->desiredSize();
    const float height = std::min(desired.y, geometry.size.y);
    out.push_back({childSlot_.widget,
                   geometry.child({left, (geometry.size.y - height) * 0.5f},
                                  {std::max(0.0f, geometry.size.x - left), height})});
}

Reply CheckBox::onMouseDown(const Geometry&, const PointerEvent& event) {
    if (event.button != cinder::platform::buttons::LEFT) return Reply::unhandled();
    pressed_ = true;
    return Reply::handled().captureMouse(shared_from_this());
}

Reply CheckBox::onMouseUp(const Geometry& geometry, const PointerEvent& event) {
    if (event.button != cinder::platform::buttons::LEFT) return Reply::unhandled();
    if (pressed_ && geometry.contains(event.position)) {
        const bool next = !checked_.get();
        if (!checked_.bound()) checked_.set(next);
        if (onChanged_) onChanged_(next);
    }
    pressed_ = false;
    return Reply::handled().releaseMouseCapture();
}

int CheckBox::onPaint(const PaintArgs& args, const Geometry& geometry, ElementList& list, int layer,
                       const PaintStyle& style, bool enabled) const {
    const CheckBoxStyle& look = Application::get().theme().get<CheckBoxStyle>(style_);
    const bool checked = checked_.get();
    const float size = look.size * geometry.scale;
    const glm::vec2 corner = geometry.position + glm::vec2(0.0f, std::round((geometry.size.y * geometry.scale - size) * 0.5f));
    const Rect box = Rect::fromSize(corner, glm::vec2(size));
    const Brush& brush = checked ? (isHovered() ? look.checkedHovered : look.checked) : (isHovered() ? look.hovered : look.box);
    PaintStyle tinted = style;
    if (!enabled) tinted.tint.a *= 0.4f;
    brush.paint(list, layer, box, tinted, geometry.scale);
    if (checked) {
        const std::array<glm::vec2, 3> mark = {box.min + glm::vec2(0.24f, 0.52f) * size,
                                               box.min + glm::vec2(0.43f, 0.70f) * size,
                                               box.min + glm::vec2(0.76f, 0.32f) * size};
        list.lines(layer + 1, mark, tinted.apply(look.mark), 1.8f * geometry.scale);
    }
    return paintChildren(args, geometry, list, layer + 1, tinted, enabled);
}

}
