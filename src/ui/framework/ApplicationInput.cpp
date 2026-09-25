#include "ui/framework/Application.hpp"
#include "ui/framework/ApplicationInternals.hpp"

#include <glm/geometric.hpp>

#include <algorithm>
#include <utility>

namespace cinder::ui {

using cinder::platform::InputEvent;
using detail::bubble;
using detail::holds;
using detail::within;

PointerEvent Application::pointer(const InputEvent& event, int button) const {
    PointerEvent result;
    result.modifiers = event.modifiers;
    result.position = cursor_;
    result.delta = cursor_ - lastCursor_;
    result.wheel = event.wheel;
    result.button = button;
    result.buttons = buttons_;
    return result;
}

WidgetPath Application::focusPath() {
    std::shared_ptr<Widget> widget = focused();
    if (!widget) return {};
    WidgetPath path = grid_.pathTo(widget.get());
    if (!path.empty()) return path;
    if (focusFresh_ || !painted_) return {PathEntry{widget, Geometry{}, widget->isEnabled()}};
    clearFocus(FocusCause::Cleared);
    return {};
}

WidgetPath Application::captorPath() const {
    std::shared_ptr<Widget> widget = captor();
    return widget ? grid_.pathTo(widget.get()) : WidgetPath{};
}

bool Application::focusWithin(const Widget& widget) const {
    std::shared_ptr<Widget> focus = focused();
    return focus && holds(grid_.pathTo(focus.get()), &widget);
}

void Application::setFocus(const std::shared_ptr<Widget>& widget, FocusCause cause) {
    Scope scope(*this);
    std::shared_ptr<Widget> previous = focused();
    if (previous == widget) return;
    focused_ = widget;
    focusFresh_ = widget != nullptr;
    if (previous) {
        previous->focused_ = false;
        previous->onFocusLost(FocusEvent{cause});
    }
    if (widget) {
        widget->focused_ = true;
        const Geometry geometry = grid_.geometryOf(widget.get()).value_or(Geometry{});
        apply(widget->onFocusReceived(geometry, FocusEvent{cause}));
    }
}

void Application::clearFocus(FocusCause cause) { setFocus(nullptr, cause); }

void Application::releaseCapture() {
    Scope scope(*this);
    std::shared_ptr<Widget> widget = captor();
    captor_.reset();
    if (!widget) return;
    widget->captured_ = false;
    widget->onMouseCaptureLost();
}

bool Application::isInteracting() const {
    if (captor() || dragDrop_) return true;
    std::shared_ptr<Widget> focus = focused();
    return focus && focus->isEditingText();
}

void Application::apply(const Reply& reply) {
    if (!reply.isHandled()) return;
    if (const std::shared_ptr<Widget>& widget = reply.captor()) {
        std::shared_ptr<Widget> previous = captor();
        if (previous && previous != widget) {
            previous->captured_ = false;
            previous->onMouseCaptureLost();
        }
        captor_ = widget;
        widget->captured_ = true;
    }
    if (reply.releasesCapture()) releaseCapture();
    if (reply.changesFocus()) {
        if (reply.focus()) setFocus(reply.focus(), FocusCause::SetDirectly);
        else clearFocus(FocusCause::Cleared);
    }
    if (reply.dragDetector()) drag_ = DragDetect{reply.dragDetector(), reply.dragButton(), cursor_};
    if (reply.dragDrop()) beginDragDrop(reply.dragDrop());
}

bool Application::navigate(bool backward) {
    std::vector<std::shared_ptr<Widget>> order = grid_.focusOrder();
    const std::shared_ptr<Widget> current = focused();
    const Widget* scope = nullptr;
    for (auto popup = popups_.rbegin(); popup != popups_.rend(); ++popup) {
        if (within(popup->content.get(), current.get())) {
            scope = popup->content.get();
            break;
        }
    }
    std::erase_if(order, [&](const std::shared_ptr<Widget>& widget) {
        if (scope != nullptr) return !within(scope, widget.get());
        return std::any_of(popups_.begin(), popups_.end(),
                           [&](const Popup& popup) { return within(popup.content.get(), widget.get()); });
    });
    if (order.empty()) return false;
    const auto found = std::find(order.begin(), order.end(), current);
    std::size_t next = 0;
    if (found == order.end()) {
        next = backward ? order.size() - 1 : 0;
    } else {
        const auto index = static_cast<std::size_t>(found - order.begin());
        next = backward ? (index + order.size() - 1) % order.size() : (index + 1) % order.size();
    }
    setFocus(order[next], FocusCause::Navigation);
    return true;
}

void Application::setHover(const WidgetPath& path, const PointerEvent& event) {
    std::vector<std::weak_ptr<Widget>> previous = std::move(hovered_);
    hovered_.clear();
    for (const std::weak_ptr<Widget>& weak : previous) {
        std::shared_ptr<Widget> widget = weak.lock();
        if (!widget || holds(path, widget.get())) continue;
        widget->hovered_ = false;
        widget->onMouseLeave(event);
    }
    for (const PathEntry& entry : path) {
        hovered_.push_back(entry.widget);
        if (entry.widget->hovered_) continue;
        entry.widget->hovered_ = true;
        entry.widget->onMouseEnter(entry.geometry, event);
    }
}

void Application::refreshHover() {
    if (!mouseEnabled_) return;
    PointerEvent event;
    event.position = cursor_;
    event.buttons = buttons_;
    event.modifiers = modifiers_;
    std::shared_ptr<Widget> widget = captor();
    if (widget) {
        WidgetPath path = grid_.pathTo(widget.get());
        if (path.empty() || !path.back().geometry.contains(cursor_)) path.clear();
        setHover(path, event);
        return;
    }
    setHover(grid_.pathAt(cursor_), event);
}

void Application::updateCursor() {
    if (!mouseEnabled_) return;
    PointerEvent event;
    event.position = cursor_;
    event.buttons = buttons_;
    event.modifiers = modifiers_;
    const WidgetPath path = captor() ? captorPath() : grid_.pathAt(cursor_);
    cinder::platform::CursorShape shape = cinder::platform::CursorShape::Arrow;
    for (auto entry = path.rbegin(); entry != path.rend(); ++entry) {
        if (auto queried = entry->widget->onCursorQuery(entry->geometry, event)) {
            shape = *queried;
            break;
        }
        if (auto fixed = entry->widget->cursor()) {
            shape = *fixed;
            break;
        }
    }
    platform_.setCursor(shape);
}

void Application::focusFromClick(const WidgetPath& path) {
    for (auto entry = path.rbegin(); entry != path.rend(); ++entry) {
        if (entry->enabled && entry->widget->supportsKeyboardFocus()) {
            setFocus(entry->widget, FocusCause::Mouse);
            return;
        }
    }
    clearFocus(FocusCause::Mouse);
}

void Application::mouseDown(const InputEvent& event) {
    const PointerEvent press = pointer(event, event.code);
    const double now = platform_.time();
    const bool doubleClick = event.code == lastClick_.button && now - lastClick_.time <= DOUBLE_CLICK_TIME
            && glm::distance(cursor_, lastClick_.position) <= DOUBLE_CLICK_DISTANCE;
    lastClick_ = doubleClick ? Click{} : Click{event.code, now, cursor_};
    toolTip_.suppressed = toolTip_.widget;
    toolTip_.shown = false;

    if (std::shared_ptr<Widget> widget = captor()) {
        const WidgetPath path = captorPath();
        const Geometry geometry = path.empty() ? Geometry{} : path.back().geometry;
        Reply reply = doubleClick ? widget->onMouseDoubleClick(geometry, press) : Reply::unhandled();
        if (!reply.isHandled()) reply = widget->onMouseDown(geometry, press);
        apply(reply);
        return;
    }

    const WidgetPath path = grid_.pathAt(cursor_);
    if (!routePopupPress(path)) {
        lastClick_ = Click{};
        return;
    }
    const std::size_t popups = popups_.size();
    Reply reply = Reply::unhandled();
    if (doubleClick) {
        reply = bubble(path, [&](const PathEntry& entry) { return entry.widget->onMouseDoubleClick(entry.geometry, press); });
    }
    if (!reply.isHandled()) {
        reply = bubble(path, [&](const PathEntry& entry) { return entry.widget->onMouseDown(entry.geometry, press); });
    }
    const bool focusChanged = (reply.isHandled() && reply.changesFocus()) || popups_.size() > popups;
    apply(reply);
    if (!focusChanged) focusFromClick(path);
}

void Application::mouseUp(const InputEvent& event) {
    const PointerEvent release = pointer(event, event.code);
    if (dragDrop_) {
        drop(event);
        if (buttons_ == 0) releaseCapture();
        return;
    }
    if (std::shared_ptr<Widget> widget = captor()) {
        const WidgetPath path = captorPath();
        const Geometry geometry = path.empty() ? Geometry{} : path.back().geometry;
        apply(widget->onMouseUp(geometry, release));
    } else {
        const WidgetPath path = grid_.pathAt(cursor_);
        apply(bubble(path, [&](const PathEntry& entry) { return entry.widget->onMouseUp(entry.geometry, release); }));
    }
    if (buttons_ == 0) {
        drag_.reset();
        releaseCapture();
    }
}

void Application::mouseMove(const InputEvent& event) {
    const PointerEvent move = pointer(event, -1);
    if (dragDrop_) {
        refreshHover();
        dragMove(event);
        return;
    }
    if (drag_ && move.isDown(drag_->button) && glm::distance(cursor_, drag_->origin) > DRAG_THRESHOLD) {
        std::shared_ptr<Widget> detector = drag_->widget.lock();
        drag_.reset();
        if (detector) {
            const Geometry geometry = grid_.geometryOf(detector.get()).value_or(Geometry{});
            apply(detector->onDragDetected(geometry, move));
        }
    }

    refreshHover();
    if (std::shared_ptr<Widget> widget = captor()) {
        const WidgetPath path = captorPath();
        const Geometry geometry = path.empty() ? Geometry{} : path.back().geometry;
        apply(widget->onMouseMove(geometry, move));
        return;
    }
    const WidgetPath path = grid_.pathAt(cursor_);
    apply(bubble(path, [&](const PathEntry& entry) { return entry.widget->onMouseMove(entry.geometry, move); }));
}

void Application::wheel(const InputEvent& event) {
    const PointerEvent scroll = pointer(event, -1);
    const WidgetPath path = captor() ? captorPath() : grid_.pathAt(cursor_);
    apply(bubble(path, [&](const PathEntry& entry) { return entry.widget->onMouseWheel(entry.geometry, scroll); }));
}

void Application::keyDown(const InputEvent& event) {
    KeyEvent key;
    key.modifiers = event.modifiers;
    key.key = event.code;
    key.repeat = event.repeat;
    const WidgetPath path = focusPath();
    const Reply reply = bubble(path, [&](const PathEntry& entry) { return entry.widget->onKeyDown(entry.geometry, key); });
    if (reply.isHandled()) {
        apply(reply);
        return;
    }
    if (event.code == cinder::platform::keys::ESCAPE && dragDrop_) {
        cancelDragDrop();
        return;
    }
    if (event.code == cinder::platform::keys::ESCAPE && !popups_.empty()) {
        dismissFrom(popups_.size() - 1);
        return;
    }
    if (event.code == cinder::platform::keys::TAB && !key.primary() && !key.control() && !key.alt()) {
        if (navigate(key.shift())) return;
    }
    if (event.repeat) return;
    for (const std::shared_ptr<const CommandList>& commands : commands_) {
        if (commands->process(key)) return;
    }
}

void Application::keyUp(const InputEvent& event) {
    KeyEvent key;
    key.modifiers = event.modifiers;
    key.key = event.code;
    const WidgetPath path = focusPath();
    apply(bubble(path, [&](const PathEntry& entry) { return entry.widget->onKeyUp(entry.geometry, key); }));
}

void Application::character(const InputEvent& event) {
    CharEvent typed;
    typed.modifiers = event.modifiers;
    typed.character = event.character;
    const WidgetPath path = focusPath();
    apply(bubble(path, [&](const PathEntry& entry) { return entry.widget->onKeyChar(entry.geometry, typed); }));
}

void Application::windowFocusLost() {
    keys_.fill(false);
    buttons_ = 0;
    drag_.reset();
    cancelDragDrop();
    releaseCapture();
    dismissAllPopups();
    setHover({}, PointerEvent{});
}

}
