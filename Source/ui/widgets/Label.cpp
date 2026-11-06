#include "ui/widgets/Label.hpp"

#include "ui/core/ElementList.hpp"
#include "ui/framework/Application.hpp"

namespace cinder::ui {

void Label::construct(const Args& args) {
    text_ = args.text_;
    textStyle_ = args.textStyle_;
    font_ = args.font_;
    color_ = args.colorAndOpacity_;
    justification_ = args.justification_ == HAlign::Fill ? HAlign::Left : args.justification_;
}

FontInfo Label::resolveFont() const {
    if (font_) return *font_;
    return Application::get().theme().get<LabelStyle>(textStyle_).font;
}

glm::vec2 Label::computeDesiredSize(float) const {
    Application& app = Application::get();
    run_.shape(app.fonts(), text_.get(), resolveFont());
    return run_.size();
}

int Label::onPaint(const PaintArgs&, const Geometry& geometry, ElementList& list, int layer,
                        const PaintStyle& style, bool enabled) const {
    Application& app = Application::get();
    run_.shape(app.fonts(), text_.get(), resolveFont());
    Color color = color_ ? color_->get() : style.foreground;
    color = style.apply(color);
    if (!enabled) color.a *= 0.4f;
    const int alignment = justification_ == HAlign::Center ? 1 : (justification_ == HAlign::Right ? 2 : 0);
    run_.paint(list, layer, geometry.position, geometry.scale, color, app.atlas(), geometry.size.x * geometry.scale,
               alignment);
    return layer;
}

int Image::onPaint(const PaintArgs&, const Geometry& geometry, ElementList& list, int layer,
                    const PaintStyle& style, bool enabled) const {
    PaintStyle tinted = style;
    tinted.tint = style.tint * color_.get();
    if (!enabled) tinted.tint.a *= 0.4f;
    image_.get().paint(list, layer, geometry.rect(), tinted, geometry.scale);
    return layer;
}

}
