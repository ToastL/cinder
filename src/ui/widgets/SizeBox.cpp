#include "ui/widgets/SizeBox.hpp"

#include <algorithm>

namespace cinder::ui {

void SizeBox::construct(const Args& args) {
    childSlot_.widget = args.content_;
    childSlot_.padding = args.padding_;
    childSlot_.hAlign = args.hAlign_;
    childSlot_.vAlign = args.vAlign_;
    width_ = args.widthOverride_;
    height_ = args.heightOverride_;
    minWidth_ = args.minDesiredWidth_;
    minHeight_ = args.minDesiredHeight_;
    maxWidth_ = args.maxDesiredWidth_;
    maxHeight_ = args.maxDesiredHeight_;
}

glm::vec2 SizeBox::computeDesiredSize(float layoutScale) const {
    glm::vec2 size = CompoundWidget::computeDesiredSize(layoutScale);
    const auto limit = [](float value, float low, float high) {
        if (low > 0.0f) value = std::max(value, low);
        if (high > 0.0f) value = std::min(value, high);
        return value;
    };
    size.x = limit(size.x, minWidth_.get(), maxWidth_.get());
    size.y = limit(size.y, minHeight_.get(), maxHeight_.get());
    if (width_.get() > 0.0f) size.x = width_.get();
    if (height_.get() > 0.0f) size.y = height_.get();
    return size;
}

glm::vec2 Scaler::computeDesiredSize(float layoutScale) const {
    return CompoundWidget::computeDesiredSize(layoutScale) * scale_.get();
}

void Scaler::arrangeChildren(const Geometry& geometry, ArrangedChildren& out) const {
    if (!childSlot_.widget || !takesSpace(childSlot_.widget->visibility())) return;
    const float scale = std::max(scale_.get(), 0.01f);
    out.push_back({childSlot_.widget, geometry.child(glm::vec2(0.0f), geometry.size / scale, scale)});
}

}
