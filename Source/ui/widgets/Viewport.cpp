#include "ui/widgets/Viewport.hpp"

#include "ui/core/ElementList.hpp"

namespace cinder::ui {

void Viewport::construct(const Args& args) {
    client_ = args.client_;
    opaque_ = args.opaque_;
    focusable_ = args.focusable_;
    childSlot_.widget = args.content_;
    if (client_) client_->widget_ = weak_from_this();
}

void Viewport::tick(const Geometry& geometry, double time, float deltaTime) {
    if (!client_) return;
    client_->onArrange(geometry);
    client_->tick(geometry, time, deltaTime);
}

Reply Viewport::onMouseDown(const Geometry& geometry, const PointerEvent& event) {
    return client_ ? client_->onMouseDown(geometry, event) : Reply::unhandled();
}

Reply Viewport::onMouseUp(const Geometry& geometry, const PointerEvent& event) {
    return client_ ? client_->onMouseUp(geometry, event) : Reply::unhandled();
}

Reply Viewport::onMouseMove(const Geometry& geometry, const PointerEvent& event) {
    return client_ ? client_->onMouseMove(geometry, event) : Reply::unhandled();
}

Reply Viewport::onMouseDoubleClick(const Geometry& geometry, const PointerEvent& event) {
    return client_ ? client_->onMouseDoubleClick(geometry, event) : Reply::unhandled();
}

Reply Viewport::onMouseWheel(const Geometry& geometry, const PointerEvent& event) {
    return client_ ? client_->onMouseWheel(geometry, event) : Reply::unhandled();
}

void Viewport::onMouseEnter(const Geometry& geometry, const PointerEvent& event) {
    if (client_) client_->onMouseEnter(geometry, event);
}

void Viewport::onMouseLeave(const PointerEvent& event) {
    if (client_) client_->onMouseLeave(event);
}

void Viewport::onMouseCaptureLost() {
    if (client_) client_->onMouseCaptureLost();
}

Reply Viewport::onKeyDown(const Geometry& geometry, const KeyEvent& event) {
    return client_ ? client_->onKeyDown(geometry, event) : Reply::unhandled();
}

Reply Viewport::onKeyUp(const Geometry& geometry, const KeyEvent& event) {
    return client_ ? client_->onKeyUp(geometry, event) : Reply::unhandled();
}

Reply Viewport::onKeyChar(const Geometry& geometry, const CharEvent& event) {
    return client_ ? client_->onKeyChar(geometry, event) : Reply::unhandled();
}

Reply Viewport::onFocusReceived(const Geometry&, const FocusEvent& event) {
    if (client_) client_->onFocusReceived(event);
    return Reply::unhandled();
}

void Viewport::onFocusLost(const FocusEvent& event) {
    if (client_) client_->onFocusLost(event);
}

std::optional<cinder::platform::CursorShape> Viewport::onCursorQuery(const Geometry& geometry,
                                                                    const PointerEvent& event) const {
    return client_ ? client_->cursor(geometry, event) : std::nullopt;
}

int Viewport::onPaint(const PaintArgs& args, const Geometry& geometry, ElementList& list, int layer,
                      const PaintStyle& style, bool enabled) const {
    const TextureRef texture = client_ ? client_->texture() : TextureRef::viewport();
    list.image(layer, geometry.rect(), texture, style.tint, glm::vec2(0.0f), glm::vec2(1.0f), opaque_);
    list.pushClip(geometry.rect());
    int highest = client_ ? client_->onPaint(geometry, list, layer + 1) : layer + 1;
    highest = paintChildren(args, geometry, list, highest + 1, style, enabled);
    list.popClip();
    return highest;
}

}
