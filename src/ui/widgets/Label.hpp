#pragma once

#include "ui/core/Args.hpp"
#include "ui/core/TextRun.hpp"
#include "ui/core/Widget.hpp"

#include <optional>
#include <string>

namespace cinder::ui {

class Label : public LeafWidget {
public:
    struct Args : ::cinder::ui::Args<Args, Label> {
        UI_ATTR(std::string, text)
        UI_ARG(std::string, textStyle, "Label")
        UI_ARG(std::optional<FontInfo>, font)
        UI_ARG(std::optional<Attribute<Color>>, colorAndOpacity)
        UI_ARG(HAlign, justification)
    };

    void construct(const Args& args);

    void setText(Attribute<std::string> text) { text_ = std::move(text); }
    std::string text() const { return text_.get(); }
    void setColorAndOpacity(std::optional<Attribute<Color>> color) { color_ = std::move(color); }

protected:
    glm::vec2 computeDesiredSize(float layoutScale) const override;
    int onPaint(const PaintArgs& args, const Geometry& geometry, ElementList& list, int layer,
                const PaintStyle& style, bool enabled) const override;

private:
    FontInfo resolveFont() const;

    Attribute<std::string> text_;
    std::string textStyle_;
    std::optional<FontInfo> font_;
    std::optional<Attribute<Color>> color_;
    HAlign justification_ = HAlign::Left;
    mutable TextRun run_;
};

class Image : public LeafWidget {
public:
    struct Args : ::cinder::ui::Args<Args, Image> {
        UI_ATTR(Brush, image)
        UI_ATTR(Color, colorAndOpacity, Color::white())
        UI_ARG(std::optional<glm::vec2>, desiredSizeOverride)
    };

    void construct(const Args& args) {
        image_ = args.image_;
        color_ = args.colorAndOpacity_;
        size_ = args.desiredSizeOverride_;
    }

    void setImage(Attribute<Brush> image) { image_ = std::move(image); }

protected:
    glm::vec2 computeDesiredSize(float) const override { return size_.value_or(image_.get().imageSize); }
    int onPaint(const PaintArgs& args, const Geometry& geometry, ElementList& list, int layer,
                const PaintStyle& style, bool enabled) const override;

private:
    Attribute<Brush> image_;
    Attribute<Color> color_{Color::white()};
    std::optional<glm::vec2> size_;
};

}
