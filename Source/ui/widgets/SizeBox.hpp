#pragma once

#include "ui/core/CompoundWidget.hpp"
#include "ui/core/Args.hpp"

namespace cinder::ui {

class SizeBox : public CompoundWidget {
public:
    struct Args : ::cinder::ui::Args<Args, SizeBox> {
        UI_ARG(HAlign, hAlign)
        UI_ARG(VAlign, vAlign)
        UI_ARG(Margin, padding)
        UI_ATTR(float, widthOverride)
        UI_ATTR(float, heightOverride)
        UI_ATTR(float, minDesiredWidth)
        UI_ATTR(float, minDesiredHeight)
        UI_ATTR(float, maxDesiredWidth)
        UI_ATTR(float, maxDesiredHeight)
        UI_CONTENT(content)
    };

    void construct(const Args& args);

protected:
    glm::vec2 computeDesiredSize(float layoutScale) const override;

private:
    Attribute<float> width_;
    Attribute<float> height_;
    Attribute<float> minWidth_;
    Attribute<float> minHeight_;
    Attribute<float> maxWidth_;
    Attribute<float> maxHeight_;
};

class Spacer : public LeafWidget {
public:
    struct Args : ::cinder::ui::Args<Args, Spacer> {
        UI_ATTR(glm::vec2, size)
    };

    void construct(const Args& args) { size_ = args.size_; }

protected:
    glm::vec2 computeDesiredSize(float) const override { return size_.get(); }
    int onPaint(const PaintArgs&, const Geometry&, ElementList&, int layer, const PaintStyle&, bool) const override {
        return layer;
    }

private:
    Attribute<glm::vec2> size_;
};

class Scaler : public CompoundWidget {
public:
    struct Args : ::cinder::ui::Args<Args, Scaler> {
        UI_ATTR(float, dpiScale, 1.0f)
        UI_CONTENT(content)
    };

    void construct(const Args& args) {
        scale_ = args.dpiScale_;
        setContent(args.content_);
    }

    void arrangeChildren(const Geometry& geometry, ArrangedChildren& out) const override;

protected:
    glm::vec2 computeDesiredSize(float layoutScale) const override;

private:
    Attribute<float> scale_{1.0f};
};

}
