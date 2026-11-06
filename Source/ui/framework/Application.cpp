#include "ui/framework/Application.hpp"
#include "ui/framework/ApplicationInternals.hpp"

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

}

Application::Scope::Scope(Application& application) : previous_(active) { active = &application; }
Application::Scope::~Scope() { active = previous_; }

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
    focusFresh_ = false;
    const PaintArgs args{&grid_, time_, deltaTime_};
    PaintStyle style;
    if (theme_.has<Color>("Color.Foreground")) style.foreground = theme_.color("Color.Foreground");
    int top = 0;
    if (root_) {
        root_->prepass(scale_);
        top = root_->paint(args, Geometry::root(windowSize_), list, 0, style, true);
    }
    for (std::size_t i = 0; i < popups_.size(); ++i) {
        const std::shared_ptr<Widget> content = popups_[i].content;
        content->prepass(scale_);
        popups_[i].rect = place(popups_[i]);
        const Rect rect = popups_[i].rect;
        top = content->paint(args, Geometry{rect.size(), rect.min, 1.0f}, list, top + 1, style, true);
    }
    if (dragDrop_) {
        if (const std::shared_ptr<Widget> decorator = dragDrop_->decorator()) {
            decorator->prepass(scale_);
            const glm::vec2 size = glm::min(decorator->desiredSize(), windowSize_);
            const glm::vec2 at = glm::round(glm::clamp(cursor_ + glm::vec2(DECORATOR_OFFSET_X, DECORATOR_OFFSET_Y),
                                                       glm::vec2(0.0f), glm::max(glm::vec2(0.0f), windowSize_ - size)));
            const PaintArgs loose{nullptr, time_, deltaTime_};
            top = decorator->paint(loose, Geometry{size, at, 1.0f}, list, top + 1, style, true);
        }
    }
    refreshHover();
    updateCursor();
    updateToolTip();
    paintToolTip(list, top + 1);
}

}
