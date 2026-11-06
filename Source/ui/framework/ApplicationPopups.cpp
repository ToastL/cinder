#include "ui/framework/Application.hpp"
#include "ui/framework/ApplicationInternals.hpp"

#include "ui/core/ElementList.hpp"

#include <glm/geometric.hpp>

#include <algorithm>
#include <utility>

namespace cinder::ui {

using cinder::platform::InputEvent;
using detail::bubble;
using detail::holds;
using detail::within;

int Application::popupIndex(const Widget* content) const {
    for (std::size_t i = 0; i < popups_.size(); ++i) {
        if (popups_[i].content.get() == content) return static_cast<int>(i);
    }
    return -1;
}

std::optional<Rect> Application::popupRect(const Widget* content) const {
    const int index = popupIndex(content);
    if (index < 0) return std::nullopt;
    return popups_[static_cast<std::size_t>(index)].rect;
}

void Application::pushPopup(std::shared_ptr<Widget> content, const Rect& anchor, PopupOptions options) {
    if (!content) return;
    Scope scope(*this);
    if (const int existing = popupIndex(content.get()); existing >= 0) dismissFrom(static_cast<std::size_t>(existing));
    Popup popup;
    popup.content = std::move(content);
    popup.anchor = anchor;
    popup.options = std::move(options);
    popup.previousFocus = focused_;
    popup.rect = Rect::fromSize(anchor.min, glm::vec2(0.0f));
    const std::shared_ptr<Widget> shown = popup.content;
    popups_.push_back(std::move(popup));
    if (popups_.back().options.focus && shown->supportsKeyboardFocus()) setFocus(shown, FocusCause::SetDirectly);
}

void Application::dismissFrom(std::size_t index) {
    if (index >= popups_.size()) return;
    Scope scope(*this);
    std::vector<Popup> closing(std::make_move_iterator(popups_.begin() + static_cast<std::ptrdiff_t>(index)),
                               std::make_move_iterator(popups_.end()));
    popups_.erase(popups_.begin() + static_cast<std::ptrdiff_t>(index), popups_.end());
    std::shared_ptr<Widget> focus = focused();
    for (auto popup = closing.rbegin(); popup != closing.rend(); ++popup) {
        if (!within(popup->content.get(), focus.get())) continue;
        std::shared_ptr<Widget> previous = popup->previousFocus.lock();
        if (previous && std::any_of(closing.begin(), closing.end(),
                                    [&](const Popup& other) { return within(other.content.get(), previous.get()); })) {
            previous.reset();
        }
        setFocus(previous, FocusCause::Cleared);
        focus = previous;
    }
    for (auto popup = closing.rbegin(); popup != closing.rend(); ++popup) {
        if (popup->options.onDismissed) popup->options.onDismissed();
    }
}

void Application::dismissPopup(const Widget* content) {
    const int index = popupIndex(content);
    if (index >= 0) dismissFrom(static_cast<std::size_t>(index));
}

void Application::dismissPopupsAbove(const Widget* content) {
    const int index = popupIndex(content);
    if (index >= 0) dismissFrom(static_cast<std::size_t>(index) + 1);
}

void Application::dismissAllPopups() { dismissFrom(0); }

bool Application::routePopupPress(const WidgetPath& path) {
    if (popups_.empty()) return true;
    for (std::size_t i = popups_.size(); i-- > 0;) {
        if (holds(path, popups_[i].content.get())) {
            dismissFrom(i + 1);
            return true;
        }
        const std::shared_ptr<Widget> owner = popups_[i].options.owner.lock();
        if (owner && holds(path, owner.get())) {
            dismissFrom(i + 1);
            return true;
        }
    }
    dismissAllPopups();
    return false;
}

Rect Application::place(const Popup& popup) const {
    glm::vec2 size = popup.content->desiredSize();
    size.x = std::max(size.x, popup.options.minWidth);
    size = glm::min(size, windowSize_);
    const Rect& anchor = popup.anchor;
    glm::vec2 at = anchor.min;
    switch (popup.options.placement) {
        case Placement::Below:
            at = {anchor.min.x, anchor.max.y};
            if (at.y + size.y > windowSize_.y && anchor.min.y - size.y >= 0.0f) at.y = anchor.min.y - size.y;
            break;
        case Placement::Right:
            at = {anchor.max.x, anchor.min.y};
            if (at.x + size.x > windowSize_.x && anchor.min.x - size.x >= 0.0f) at.x = anchor.min.x - size.x;
            break;
        case Placement::AtPoint:
            if (at.x + size.x > windowSize_.x) at.x -= size.x;
            if (at.y + size.y > windowSize_.y) at.y -= size.y;
            break;
        case Placement::Center:
            at = (windowSize_ - size) * 0.5f;
            break;
    }
    at = glm::round(glm::clamp(at, glm::vec2(0.0f), glm::max(glm::vec2(0.0f), windowSize_ - size)));
    return Rect::fromSize(at, size);
}

void Application::updateToolTip() {
    std::shared_ptr<Widget> owner;
    std::string text;
    if (buttons_ == 0 && mouseEnabled_ && !captor()) {
        for (auto weak = hovered_.rbegin(); weak != hovered_.rend(); ++weak) {
            std::shared_ptr<Widget> widget = weak->lock();
            if (!widget) continue;
            text = widget->toolTipText();
            if (!text.empty()) {
                owner = widget;
                break;
            }
        }
    }
    if (const std::shared_ptr<Widget> suppressed = toolTip_.suppressed.lock()) {
        const bool stillHovered = std::any_of(hovered_.begin(), hovered_.end(),
                                              [&](const std::weak_ptr<Widget>& weak) { return weak.lock() == suppressed; });
        if (!stillHovered) toolTip_.suppressed.reset();
    }
    const std::shared_ptr<Widget> previous = toolTip_.widget.lock();
    if (owner != previous) {
        toolTip_.widget = owner;
        toolTip_.since = time_;
        toolTip_.shown = false;
    }
    toolTip_.text = std::move(text);
    if (!owner || toolTip_.suppressed.lock() == owner) {
        toolTip_.shown = false;
        return;
    }
    if (!toolTip_.shown && time_ - toolTip_.since >= TOOLTIP_DELAY) {
        toolTip_.shown = true;
        toolTip_.at = cursor_;
    }
}

std::string Application::visibleToolTip() const { return toolTip_.shown ? toolTip_.text : std::string(); }

void Application::paintToolTip(ElementList& list, int layer) {
    if (!toolTip_.shown || toolTip_.text.empty()) return;
    const ToolTipStyle& look = theme_.get<ToolTipStyle>("ToolTip");
    toolTipRun_.shape(fonts_, toolTip_.text, look.font);
    const glm::vec2 size = glm::min(toolTipRun_.size() + look.padding.total(), windowSize_);
    glm::vec2 at = toolTip_.at + glm::vec2(TOOLTIP_OFFSET_X, TOOLTIP_OFFSET_Y);
    if (at.x + size.x > windowSize_.x) at.x = windowSize_.x - size.x;
    if (at.y + size.y > windowSize_.y) at.y = toolTip_.at.y - size.y - 4.0f;
    at = glm::round(glm::max(at, glm::vec2(0.0f)));
    const Rect rect = Rect::fromSize(at, size);
    look.background.paint(list, layer, rect, PaintStyle{});
    toolTipRun_.paint(list, layer + 1, at + look.padding.topLeft(), 1.0f, look.color, atlas_);
}

}
