#include "ui/framework/Application.hpp"

#include "ui/core/ElementList.hpp"

#include <glm/geometric.hpp>

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace cinder::ui {

using cinder::platform::InputEvent;
using cinder::platform::InputEventType;

namespace {

thread_local Application* active = nullptr;

template <typename Handler>
Reply bubble(const WidgetPath& path, Handler handler) {
    for (auto entry = path.rbegin(); entry != path.rend(); ++entry) {
        if (!entry->enabled) continue;
        Reply reply = handler(*entry);
        if (reply.isHandled()) return reply;
    }
    return Reply::unhandled();
}

bool holds(const WidgetPath& path, const Widget* widget) {
    return std::any_of(path.begin(), path.end(), [widget](const PathEntry& entry) { return entry.widget.get() == widget; });
}

}

class Application::Scope {
public:
    explicit Scope(Application& application) : previous_(active) { active = &application; }
    ~Scope() { active = previous_; }

    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;

private:
    Application* previous_;
};

Application::Application(PlatformHooks& platform, const cinder::text::FontSet& fonts, Theme theme)
    : platform_(platform), fonts_(fonts), theme_(std::move(theme)) {}

Application::~Application() {
    if (active == this) active = nullptr;
}

Application& Application::get() {
    if (active == nullptr) throw std::logic_error("no ui::Application is active");
    return *active;
}

Application* Application::current() { return active; }

bool Application::keyHeld(int key) const {
    return key >= 0 && key < cinder::platform::keys::COUNT && keys_[static_cast<std::size_t>(key)];
}

void Application::addCommands(std::shared_ptr<const CommandList> commands) {
    commands_.push_back(std::move(commands));
}

void Application::setMouseEnabled(bool enabled) {
    if (enabled == mouseEnabled_) return;
    mouseEnabled_ = enabled;
    if (!enabled) {
        releaseCapture();
        drag_.reset();
        setHover({}, PointerEvent{});
    }
}

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
    if (path.empty() && painted_) clearFocus(FocusCause::Cleared);
    return path;
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
    std::shared_ptr<Widget> previous = focused();
    if (previous == widget) return;
    focused_ = widget;
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
    std::shared_ptr<Widget> widget = captor();
    captor_.reset();
    if (!widget) return;
    widget->captured_ = false;
    widget->onMouseCaptureLost();
}

bool Application::isInteracting() const {
    if (captor()) return true;
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

    if (std::shared_ptr<Widget> widget = captor()) {
        const WidgetPath path = captorPath();
        const Geometry geometry = path.empty() ? Geometry{} : path.back().geometry;
        Reply reply = doubleClick ? widget->onMouseDoubleClick(geometry, press) : Reply::unhandled();
        if (!reply.isHandled()) reply = widget->onMouseDown(geometry, press);
        apply(reply);
        return;
    }

    const WidgetPath path = grid_.pathAt(cursor_);
    Reply reply = Reply::unhandled();
    if (doubleClick) {
        reply = bubble(path, [&](const PathEntry& entry) { return entry.widget->onMouseDoubleClick(entry.geometry, press); });
    }
    if (!reply.isHandled()) {
        reply = bubble(path, [&](const PathEntry& entry) { return entry.widget->onMouseDown(entry.geometry, press); });
    }
    const bool focusChanged = reply.isHandled() && reply.changesFocus();
    apply(reply);
    if (!focusChanged) focusFromClick(path);
}

void Application::mouseUp(const InputEvent& event) {
    const PointerEvent release = pointer(event, event.code);
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
    releaseCapture();
    setHover({}, PointerEvent{});
}

void Application::processEvents(std::span<const InputEvent> events) {
    Scope scope(*this);
    for (const InputEvent& event : events) {
        modifiers_ = event.modifiers;
        switch (event.type) {
            case InputEventType::KeyDown:
                if (event.code >= 0 && event.code < cinder::platform::keys::COUNT) {
                    keys_[static_cast<std::size_t>(event.code)] = true;
                }
                keyDown(event);
                break;
            case InputEventType::KeyUp:
                if (event.code >= 0 && event.code < cinder::platform::keys::COUNT) {
                    keys_[static_cast<std::size_t>(event.code)] = false;
                }
                keyUp(event);
                break;
            case InputEventType::Char:
                character(event);
                break;
            case InputEventType::MouseDown:
                lastCursor_ = cursor_;
                cursor_ = event.position;
                buttons_ |= 1u << static_cast<unsigned>(event.code);
                if (mouseEnabled_) mouseDown(event);
                break;
            case InputEventType::MouseUp:
                lastCursor_ = cursor_;
                cursor_ = event.position;
                buttons_ &= ~(1u << static_cast<unsigned>(event.code));
                if (mouseEnabled_) mouseUp(event);
                break;
            case InputEventType::MouseMove:
                lastCursor_ = cursor_;
                cursor_ = event.position;
                if (mouseEnabled_) mouseMove(event);
                break;
            case InputEventType::Wheel:
                if (mouseEnabled_) wheel(event);
                break;
            case InputEventType::CursorEnter:
                break;
            case InputEventType::CursorLeave:
                setHover({}, pointer(event, -1));
                break;
            case InputEventType::FocusLost:
                windowFocusLost();
                break;
        }
    }
    updateCursor();
}

void Application::paint(ElementList& list) {
    Scope scope(*this);
    const double now = platform_.time();
    deltaTime_ = painted_ ? static_cast<float>(std::max(0.0, now - time_)) : 0.0f;
    time_ = now;
    windowSize_ = list.size();
    scale_ = list.scale();
    if (atlasScale_ != scale_) {
        atlas_.clear();
        atlasScale_ = scale_;
    }

    grid_.clear();
    painted_ = true;
    if (!root_) return;
    root_->prepass(scale_);
    const PaintArgs args{&grid_, time_, deltaTime_};
    PaintStyle style;
    if (theme_.has<Color>("Color.Foreground")) style.foreground = theme_.color("Color.Foreground");
    root_->paint(args, Geometry::root(windowSize_), list, 0, style, true);
    refreshHover();
    updateCursor();
}

}
