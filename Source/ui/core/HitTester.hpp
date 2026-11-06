#pragma once

#include "ui/core/Geometry.hpp"
#include "ui/core/Rect.hpp"
#include "ui/core/Widget.hpp"

#include <glm/vec2.hpp>

#include <memory>
#include <optional>
#include <vector>

namespace cinder::ui {

struct PathEntry {
    std::shared_ptr<Widget> widget;
    Geometry geometry;
    bool enabled = true;
};

using WidgetPath = std::vector<PathEntry>;

class HitTester {
public:
    void clear();

    void push(Widget& widget, const Geometry& geometry, const Rect& clip, int layer, Visibility visibility,
              bool enabled);
    void pop();

    WidgetPath pathAt(glm::vec2 point) const;
    WidgetPath pathTo(const Widget* widget) const;
    std::optional<Geometry> geometryOf(const Widget* widget) const;
    bool contains(const Widget* widget) const;
    std::vector<std::shared_ptr<Widget>> focusOrder() const;
    std::size_t size() const { return entries_.size(); }

private:
    struct Entry {
        std::weak_ptr<Widget> widget;
        const Widget* raw = nullptr;
        Geometry geometry;
        Rect clip;
        int layer = 0;
        int parent = -1;
        bool hittable = false;
        bool childrenHittable = false;
        bool enabled = true;
    };

    WidgetPath build(int index) const;
    int find(const Widget* widget) const;

    std::vector<Entry> entries_;
    std::vector<int> stack_;
};

}
