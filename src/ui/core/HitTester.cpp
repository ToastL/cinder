#include "ui/core/HitTester.hpp"

#include <algorithm>

namespace cinder::ui {

void HitTester::clear() {
    entries_.clear();
    stack_.clear();
}

void HitTester::push(Widget& widget, const Geometry& geometry, const Rect& clip, int layer, Visibility visibility,
                       bool enabled) {
    Entry entry;
    entry.widget = widget.weak_from_this();
    entry.raw = &widget;
    entry.geometry = geometry;
    entry.clip = clip;
    entry.layer = layer;
    entry.parent = stack_.empty() ? -1 : stack_.back();
    entry.enabled = enabled;
    const bool allowed = entry.parent < 0 || entries_[static_cast<std::size_t>(entry.parent)].childrenHittable;
    entry.hittable = allowed && hitsSelf(visibility);
    entry.childrenHittable = allowed && hitsChildren(visibility);
    entries_.push_back(entry);
    stack_.push_back(static_cast<int>(entries_.size()) - 1);
}

void HitTester::pop() {
    if (!stack_.empty()) stack_.pop_back();
}

WidgetPath HitTester::build(int index) const {
    WidgetPath path;
    for (int at = index; at >= 0; at = entries_[static_cast<std::size_t>(at)].parent) {
        const Entry& entry = entries_[static_cast<std::size_t>(at)];
        std::shared_ptr<Widget> widget = entry.widget.lock();
        if (!widget) return {};
        path.push_back(PathEntry{std::move(widget), entry.geometry, entry.enabled});
    }
    std::reverse(path.begin(), path.end());
    return path;
}

WidgetPath HitTester::pathAt(glm::vec2 point) const {
    int best = -1;
    for (int i = 0; i < static_cast<int>(entries_.size()); ++i) {
        const Entry& entry = entries_[static_cast<std::size_t>(i)];
        if (!entry.hittable || !entry.clip.contains(point) || !entry.geometry.contains(point)) continue;
        if (best < 0 || entry.layer >= entries_[static_cast<std::size_t>(best)].layer) best = i;
    }
    return best < 0 ? WidgetPath{} : build(best);
}

int HitTester::find(const Widget* widget) const {
    for (int i = static_cast<int>(entries_.size()) - 1; i >= 0; --i) {
        if (entries_[static_cast<std::size_t>(i)].raw == widget) return i;
    }
    return -1;
}

WidgetPath HitTester::pathTo(const Widget* widget) const {
    const int index = find(widget);
    return index < 0 ? WidgetPath{} : build(index);
}

std::optional<Geometry> HitTester::geometryOf(const Widget* widget) const {
    const int index = find(widget);
    if (index < 0) return std::nullopt;
    return entries_[static_cast<std::size_t>(index)].geometry;
}

bool HitTester::contains(const Widget* widget) const { return find(widget) >= 0; }

}
