#pragma once

#include "ui/core/Args.hpp"
#include "ui/core/CompoundWidget.hpp"
#include "ui/core/Delegates.hpp"

#include <glm/vec3.hpp>

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace cinder::ui {

class Application;

using OnColorChanged = std::function<void(Color)>;

glm::vec3 rgbToHsv(glm::vec3 rgb);
glm::vec3 hsvToRgb(glm::vec3 hsv);
std::string toHex(Color linear, bool alpha);
std::optional<Color> fromHex(std::string_view text);

class ColorBlock : public LeafWidget {
public:
    struct Args : ::cinder::ui::Args<Args, ColorBlock> {
        UI_ATTR(Color, color, Color::white())
        UI_ARG(bool, showAlpha, true)
        UI_ARG(glm::vec2, size, glm::vec2(48.0f, 18.0f))
        UI_ARG(bool, opensPicker)
        UI_ARG(bool, useAlpha, true)
        UI_ARG(std::string, style, "ColorPicker")
        UI_EVENT(OnColorChanged, onColorChanged)
        UI_EVENT(OnClicked, onClicked)
    };

    void construct(const Args& args);

    Color color() const { return color_.get(); }
    void change(Color color);
    bool isPickerOpen() const;
    void openPicker();
    const std::shared_ptr<Widget>& picker() const { return picker_; }

    Reply onMouseDown(const Geometry& geometry, const PointerEvent& event) override;

protected:
    glm::vec2 computeDesiredSize(float) const override { return size_; }
    int onPaint(const PaintArgs& args, const Geometry& geometry, ElementList& list, int layer,
                const PaintStyle& style, bool enabled) const override;

private:
    Attribute<Color> color_{Color::white()};
    bool showAlpha_ = true;
    glm::vec2 size_{48.0f, 18.0f};
    bool opensPicker_ = false;
    bool useAlpha_ = true;
    std::string style_;
    OnColorChanged onChanged_;
    OnClicked onClicked_;
    std::shared_ptr<Widget> picker_;
    Application* app_ = nullptr;
};

class ColorPicker : public CompoundWidget {
public:
    static constexpr float AREA = 176.0f;
    static constexpr float BAR = 16.0f;

    struct Args : ::cinder::ui::Args<Args, ColorPicker> {
        UI_ATTR(Color, color, Color::white())
        UI_ARG(bool, useAlpha, true)
        UI_ARG(std::string, style, "ColorPicker")
        UI_EVENT(OnColorChanged, onColorChanged)
    };

    void construct(const Args& args);

    Color color() const { return color_.get(); }
    Color original() const { return original_; }
    void setColor(Color linear);
    glm::vec3 hsv() const;
    void setHsv(glm::vec3 hsv);
    void setAlpha(float alpha);
    bool useAlpha() const { return useAlpha_; }
    const std::string& style() const { return style_; }

private:
    Attribute<Color> color_{Color::white()};
    Color original_;
    bool useAlpha_ = true;
    std::string style_;
    OnColorChanged onChanged_;
    mutable glm::vec3 hsv_{0.0f};
    mutable std::optional<Color> hsvFor_;
};

}
