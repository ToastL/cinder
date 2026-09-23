#pragma once

#include "ui/core/Args.hpp"
#include "ui/core/CompoundWidget.hpp"
#include "ui/core/Delegates.hpp"

#include <memory>
#include <string>

namespace cinder::ui {

class ExpanderArrow : public LeafWidget {
public:
    struct Args : ::cinder::ui::Args<Args, ExpanderArrow> {
        UI_ATTR(bool, expanded)
        UI_ARG(float, size, 10.0f)
        UI_EVENT(OnVoid, onToggled)
    };

    void construct(const Args& args);

    Reply onMouseDown(const Geometry& geometry, const PointerEvent& event) override;
    Reply onMouseDoubleClick(const Geometry& geometry, const PointerEvent& event) override;

    static void paintArrow(ElementList& list, int layer, glm::vec2 centre, float size, bool expanded, Color color);

protected:
    glm::vec2 computeDesiredSize(float layoutScale) const override { return glm::vec2(size_); }
    int onPaint(const PaintArgs& args, const Geometry& geometry, ElementList& list, int layer,
                const PaintStyle& style, bool enabled) const override;

private:
    Attribute<bool> expanded_;
    float size_ = 10.0f;
    OnVoid onToggled_;
};

class ExpandableArea : public CompoundWidget {
public:
    struct Args : ::cinder::ui::Args<Args, ExpandableArea> {
        UI_ATTR(std::string, areaTitle)
        UI_ARG(std::string, titleStyle, "Label.Bold")
        UI_ARG(std::shared_ptr<Widget>, headerContent)
        UI_ARG(std::shared_ptr<Widget>, bodyContent)
        UI_ARG(bool, initiallyCollapsed)
        UI_ARG(Margin, padding)
        UI_ARG(Margin, headerPadding, Margin(6.0f, 4.0f))
        UI_ARG(std::string, style, "ExpandableArea")
        UI_EVENT(OnBoolChanged, onAreaExpansionChanged)
    };

    void construct(const Args& args);

    bool isExpanded() const { return expanded_; }
    void setExpanded(bool expanded);
    const std::shared_ptr<Widget>& header() const { return header_; }
    const std::shared_ptr<Widget>& body() const { return body_; }

private:
    std::shared_ptr<Widget> header_;
    std::shared_ptr<Widget> body_;
    std::string style_;
    OnBoolChanged onChanged_;
    bool expanded_ = true;
};

}
