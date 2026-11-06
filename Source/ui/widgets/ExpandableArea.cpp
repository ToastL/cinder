#include "ui/widgets/ExpandableArea.hpp"

#include "ui/core/ElementList.hpp"
#include "ui/framework/Application.hpp"
#include "ui/widgets/Border.hpp"
#include "ui/widgets/BoxPanel.hpp"
#include "ui/widgets/Label.hpp"

#include <array>
#include <cmath>

namespace cinder::ui {

void ExpanderArrow::construct(const Args& args) {
    expanded_ = args.expanded_;
    size_ = args.size_;
    onToggled_ = args.onToggled_;
}

Reply ExpanderArrow::onMouseDown(const Geometry&, const PointerEvent& event) {
    if (!onToggled_ || event.button != cinder::platform::buttons::LEFT) return Reply::unhandled();
    onToggled_();
    return Reply::handled();
}

Reply ExpanderArrow::onMouseDoubleClick(const Geometry& geometry, const PointerEvent& event) {
    return onMouseDown(geometry, event);
}

void ExpanderArrow::paintArrow(ElementList& list, int layer, glm::vec2 centre, float size, bool expanded, Color color) {
    const float half = size * 0.5f;
    const float depth = size * 0.35f;
    std::array<glm::vec2, 3> points;
    if (expanded) {
        points = {centre + glm::vec2(-half * 0.8f, -depth * 0.6f), centre + glm::vec2(half * 0.8f, -depth * 0.6f),
                  centre + glm::vec2(0.0f, depth * 1.1f)};
    } else {
        points = {centre + glm::vec2(-depth * 0.6f, -half * 0.8f), centre + glm::vec2(depth * 1.1f, 0.0f),
                  centre + glm::vec2(-depth * 0.6f, half * 0.8f)};
    }
    list.polygon(layer, points, color);
}

int ExpanderArrow::onPaint(const PaintArgs&, const Geometry& geometry, ElementList& list, int layer,
                           const PaintStyle& style, bool enabled) const {
    Color color = style.apply(style.foreground);
    if (!isHovered()) color.a *= 0.75f;
    if (!enabled) color.a *= 0.4f;
    const glm::vec2 centre = geometry.absolute(geometry.size * 0.5f);
    paintArrow(list, layer, centre, size_ * geometry.scale, expanded_.get(), color);
    return layer;
}

void ExpandableArea::construct(const Args& args) {
    style_ = args.style_;
    onChanged_ = args.onAreaExpansionChanged_;
    expanded_ = !args.initiallyCollapsed_;

    std::shared_ptr<Widget> title = args.headerContent_
            ? args.headerContent_
            : std::shared_ptr<Widget>(make<Label>().text(args.areaTitle_).textStyle(args.titleStyle_));

    std::shared_ptr<Border> header = make<Border>()
            .padding(args.headerPadding_)
            .onMouseDown([this](const Geometry&, const PointerEvent& event) {
                if (event.button != cinder::platform::buttons::LEFT) return Reply::unhandled();
                setExpanded(!expanded_);
                return Reply::handled();
            })
            [make<HorizontalBox>()
             + HorizontalBox::slot().autoWidth().vAlign(VAlign::Center).padding(Margin(0.0f, 0.0f, 4.0f, 0.0f))
                   [make<ExpanderArrow>().expanded([this] { return expanded_; })]
             + HorizontalBox::slot().fill(1.0f).vAlign(VAlign::Center)[title]];
    header->setBrush([this] {
        const ExpandableAreaStyle& look = Application::get().theme().get<ExpandableAreaStyle>(style_);
        return header_ && header_->isHovered() ? look.headerHovered : look.header;
    });
    header_ = header;

    body_ = make<Border>()
            .padding(args.padding_)
            .brush([this] { return Application::get().theme().get<ExpandableAreaStyle>(style_).body; })
            .visibility([this] { return expanded_ ? Visibility::Visible : Visibility::Collapsed; })
            [args.bodyContent_];

    childSlot_.widget = make<VerticalBox>()
            + VerticalBox::slot().autoHeight()[header_]
            + VerticalBox::slot().autoHeight()[body_];
}

void ExpandableArea::setExpanded(bool expanded) {
    if (expanded == expanded_) return;
    expanded_ = expanded;
    if (onChanged_) onChanged_(expanded_);
}

}
