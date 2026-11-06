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

void Application::beginDragDrop(std::shared_ptr<DragDropOperation> operation) {
    if (dragDrop_) cancelDragDrop();
    releaseCapture();
    drag_.reset();
    dragDrop_ = std::move(operation);
    toolTip_.shown = false;
    InputEvent here;
    here.position = cursor_;
    here.modifiers = modifiers_;
    dragMove(here);
}

DragDropEvent Application::dragEvent(const InputEvent& event) const {
    DragDropEvent result;
    static_cast<PointerEvent&>(result) = pointer(event, event.code);
    result.operation = dragDrop_;
    return result;
}

void Application::leaveDragTargets(const WidgetPath& keep, const DragDropEvent& event) {
    std::vector<std::weak_ptr<Widget>> previous = std::move(dragTargets_);
    dragTargets_.clear();
    for (const std::weak_ptr<Widget>& weak : previous) {
        std::shared_ptr<Widget> widget = weak.lock();
        if (!widget) continue;
        if (holds(keep, widget.get())) dragTargets_.push_back(widget);
        else widget->onDragLeave(event);
    }
}

void Application::dragMove(const InputEvent& event) {
    if (!dragDrop_) return;
    const DragDropEvent over = dragEvent(event);
    const WidgetPath path = grid_.pathAt(cursor_);
    leaveDragTargets(path, over);
    for (const PathEntry& entry : path) {
        const bool known = std::any_of(dragTargets_.begin(), dragTargets_.end(),
                                       [&](const std::weak_ptr<Widget>& weak) { return weak.lock() == entry.widget; });
        if (known) continue;
        dragTargets_.push_back(entry.widget);
        if (entry.enabled) entry.widget->onDragEnter(entry.geometry, over);
    }
    bubble(path, [&](const PathEntry& entry) { return entry.widget->onDragOver(entry.geometry, over); });
}

void Application::drop(const InputEvent& event) {
    const std::shared_ptr<DragDropOperation> operation = dragDrop_;
    const DragDropEvent dropped = dragEvent(event);
    const WidgetPath path = grid_.pathAt(cursor_);
    const Reply reply = bubble(path, [&](const PathEntry& entry) { return entry.widget->onDrop(entry.geometry, dropped); });
    leaveDragTargets({}, dropped);
    dragDrop_.reset();
    operation->onDropped(reply.isHandled());
    apply(reply);
}

void Application::cancelDragDrop() {
    if (!dragDrop_) return;
    const std::shared_ptr<DragDropOperation> operation = dragDrop_;
    DragDropEvent event;
    event.position = cursor_;
    event.operation = operation;
    leaveDragTargets({}, event);
    dragDrop_.reset();
    operation->onDropped(false);
}

}
