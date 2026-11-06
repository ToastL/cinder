#pragma once

#include "ui/core/Layout.hpp"
#include "ui/core/Widget.hpp"

#include <memory>
#include <optional>

namespace cinder::ui {

struct ChildSlot {
    std::shared_ptr<Widget> widget;
    Margin padding;
    HAlign hAlign = HAlign::Fill;
    VAlign vAlign = VAlign::Fill;
};

class CompoundWidget : public Widget {
public:
    int childCount() const override { return childSlot_.widget ? 1 : 0; }
    Widget* childAt(int index) const override { return index == 0 ? childSlot_.widget.get() : nullptr; }
    void arrangeChildren(const Geometry& geometry, ArrangedChildren& out) const override;

    void setContent(std::shared_ptr<Widget> content) { childSlot_.widget = std::move(content); }
    const std::shared_ptr<Widget>& content() const { return childSlot_.widget; }

    void setColorAndOpacity(Attribute<Color> color) { colorAndOpacity_ = std::move(color); }
    void setForegroundColor(std::optional<Attribute<Color>> color) { foreground_ = std::move(color); }

protected:
    glm::vec2 computeDesiredSize(float layoutScale) const override;
    int onPaint(const PaintArgs& args, const Geometry& geometry, ElementList& list, int layer,
                const PaintStyle& style, bool enabled) const override;

    PaintStyle childStyle(const PaintStyle& style) const;

    ChildSlot childSlot_;
    Attribute<Color> colorAndOpacity_{Color::white()};
    std::optional<Attribute<Color>> foreground_;
};

}
