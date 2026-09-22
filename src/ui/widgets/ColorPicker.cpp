#include "ui/widgets/ColorPicker.hpp"

#include "ui/core/ElementList.hpp"
#include "ui/framework/Application.hpp"
#include "ui/widgets/Border.hpp"
#include "ui/widgets/BoxPanel.hpp"
#include "ui/widgets/Label.hpp"
#include "ui/widgets/TextField.hpp"
#include "ui/widgets/VectorInputBox.hpp"

#include <glm/common.hpp>
#include <glm/trigonometric.hpp>

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <vector>

namespace cinder::ui {

namespace {

constexpr int GRID = 12;
constexpr int HUE_STEPS = 24;
constexpr int MARKER_POINTS = 24;

Color linearFromSrgb(glm::vec3 srgb, float alpha) {
    return {Color::toLinear(srgb.r), Color::toLinear(srgb.g), Color::toLinear(srgb.b), alpha};
}

glm::vec3 srgbOf(Color linear) {
    return glm::clamp(glm::vec3(Color::toSrgb(std::max(0.0f, linear.r)), Color::toSrgb(std::max(0.0f, linear.g)),
                                Color::toSrgb(std::max(0.0f, linear.b))),
                      glm::vec3(0.0f), glm::vec3(1.0f));
}

void paintChecker(ElementList& list, int layer, const Rect& rect, const ColorPickerStyle& look, const PaintStyle& style) {
    list.box(layer, rect, BoxStyle{style.apply(look.checkerLight)});
    const float cell = std::max(2.0f, look.checkerSize);
    list.pushClip(rect);
    int row = 0;
    for (float y = rect.min.y; y < rect.max.y; y += cell, ++row) {
        int column = 0;
        for (float x = rect.min.x; x < rect.max.x; x += cell, ++column) {
            if (((row + column) & 1) == 0) continue;
            list.box(layer, Rect{{x, y}, {std::min(x + cell, rect.max.x), std::min(y + cell, rect.max.y)}},
                     BoxStyle{style.apply(look.checkerDark)});
        }
    }
    list.popClip();
}

void paintRing(ElementList& list, int layer, glm::vec2 centre, float radius, Color inner, Color outer) {
    std::array<glm::vec2, MARKER_POINTS> points;
    for (int i = 0; i < MARKER_POINTS; ++i) {
        const float angle = glm::radians(360.0f * static_cast<float>(i) / MARKER_POINTS);
        points[static_cast<std::size_t>(i)] = centre + glm::vec2(std::cos(angle), std::sin(angle)) * radius;
    }
    list.lines(layer, points, outer, 3.0f, true);
    list.lines(layer + 1, points, inner, 1.5f, true);
}

class PickerArea : public LeafWidget {
public:
    enum class Kind { SaturationValue, Hue, Alpha };

    PickerArea(ColorPicker& picker, Kind kind) : picker_(&picker), kind_(kind) {}

    Reply onMouseDown(const Geometry& geometry, const PointerEvent& event) override {
        if (event.button != cinder::platform::buttons::LEFT) return Reply::unhandled();
        dragging_ = true;
        pick(geometry, event.position);
        return Reply::handled().captureMouse(shared_from_this());
    }

    Reply onMouseMove(const Geometry& geometry, const PointerEvent& event) override {
        if (!dragging_) return Reply::unhandled();
        pick(geometry, event.position);
        return Reply::handled();
    }

    Reply onMouseUp(const Geometry&, const PointerEvent& event) override {
        if (!dragging_ || event.button != cinder::platform::buttons::LEFT) return Reply::unhandled();
        dragging_ = false;
        return Reply::handled().releaseMouseCapture();
    }

    void onMouseCaptureLost() override { dragging_ = false; }

    std::optional<cinder::platform::CursorShape> onCursorQuery(const Geometry&, const PointerEvent&) const override {
        return cinder::platform::CursorShape::Crosshair;
    }

protected:
    glm::vec2 computeDesiredSize(float) const override {
        return kind_ == Kind::SaturationValue ? glm::vec2(ColorPicker::AREA) : glm::vec2(ColorPicker::BAR, ColorPicker::AREA);
    }

    int onPaint(const PaintArgs&, const Geometry& geometry, ElementList& list, int layer, const PaintStyle& style,
                bool) const override {
        const ColorPickerStyle& look = Application::get().theme().get<ColorPickerStyle>(picker_->style());
        const Rect rect = geometry.rect();
        const glm::vec3 hsv = picker_->hsv();
        const Color marker = style.apply(look.marker);
        const Color shadow = style.apply(Color{0.0f, 0.0f, 0.0f, 0.7f});

        if (kind_ == Kind::SaturationValue) {
            std::vector<glm::vec2> positions;
            std::vector<Color> colors;
            std::vector<std::uint32_t> indices;
            for (int y = 0; y <= GRID; ++y) {
                for (int x = 0; x <= GRID; ++x) {
                    const glm::vec2 unit(static_cast<float>(x) / GRID, static_cast<float>(y) / GRID);
                    positions.push_back(glm::mix(rect.min, rect.max, unit));
                    colors.push_back(style.apply(linearFromSrgb(hsvToRgb({hsv.x, unit.x, 1.0f - unit.y}), 1.0f)));
                }
            }
            for (int y = 0; y < GRID; ++y) {
                for (int x = 0; x < GRID; ++x) {
                    const auto corner = static_cast<std::uint32_t>(y * (GRID + 1) + x);
                    const auto below = corner + static_cast<std::uint32_t>(GRID + 1);
                    indices.insert(indices.end(), {corner, corner + 1, below + 1, corner, below + 1, below});
                }
            }
            list.mesh(layer, positions, colors, indices);
            const glm::vec2 at = glm::mix(rect.min, rect.max, glm::vec2(hsv.y, 1.0f - hsv.z));
            list.pushClip(rect);
            paintRing(list, layer + 1, at, 5.0f, marker, shadow);
            list.popClip();
            return layer + 2;
        }

        float fraction = 0.0f;
        if (kind_ == Kind::Hue) {
            for (int i = 0; i < HUE_STEPS; ++i) {
                const float top = static_cast<float>(i) / HUE_STEPS;
                const float bottom = static_cast<float>(i + 1) / HUE_STEPS;
                const Color a = style.apply(linearFromSrgb(hsvToRgb({top, 1.0f, 1.0f}), 1.0f));
                const Color b = style.apply(linearFromSrgb(hsvToRgb({bottom, 1.0f, 1.0f}), 1.0f));
                list.gradient(layer, Rect{{rect.min.x, glm::mix(rect.min.y, rect.max.y, top)},
                                          {rect.max.x, glm::mix(rect.min.y, rect.max.y, bottom)}},
                              a, a, b, b);
            }
            fraction = hsv.x;
        } else {
            paintChecker(list, layer, rect, look, style);
            const Color solid = picker_->color();
            const Color clear{solid.r, solid.g, solid.b, 0.0f};
            const Color opaque{solid.r, solid.g, solid.b, 1.0f};
            list.gradient(layer + 1, rect, style.apply(opaque), style.apply(opaque), style.apply(clear), style.apply(clear));
            fraction = 1.0f - std::clamp(solid.a, 0.0f, 1.0f);
        }
        const float y = std::round(glm::mix(rect.min.y, rect.max.y, fraction));
        const Rect bar{{rect.min.x - 2.0f, y - 2.0f}, {rect.max.x + 2.0f, y + 2.0f}};
        list.box(layer + 2, bar, BoxStyle{Color::transparent(), marker, 1.5f, glm::vec4(2.0f)});
        return layer + 2;
    }

private:
    void pick(const Geometry& geometry, glm::vec2 position) {
        const glm::vec2 unit = glm::clamp(geometry.local(position) / glm::max(geometry.size, glm::vec2(1.0f)),
                                          glm::vec2(0.0f), glm::vec2(1.0f));
        glm::vec3 hsv = picker_->hsv();
        switch (kind_) {
            case Kind::SaturationValue:
                hsv.y = unit.x;
                hsv.z = 1.0f - unit.y;
                picker_->setHsv(hsv);
                break;
            case Kind::Hue:
                hsv.x = std::min(unit.y, 0.9999f);
                picker_->setHsv(hsv);
                break;
            case Kind::Alpha:
                picker_->setAlpha(1.0f - unit.y);
                break;
        }
    }

    ColorPicker* picker_;
    Kind kind_;
    bool dragging_ = false;
};

int hexDigit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

}

glm::vec3 rgbToHsv(glm::vec3 rgb) {
    const float high = std::max({rgb.r, rgb.g, rgb.b});
    const float low = std::min({rgb.r, rgb.g, rgb.b});
    const float range = high - low;
    float hue = 0.0f;
    if (range > 0.0f) {
        if (high == rgb.r) hue = std::fmod((rgb.g - rgb.b) / range, 6.0f);
        else if (high == rgb.g) hue = (rgb.b - rgb.r) / range + 2.0f;
        else hue = (rgb.r - rgb.g) / range + 4.0f;
        hue /= 6.0f;
        if (hue < 0.0f) hue += 1.0f;
    }
    const float saturation = high > 0.0f ? range / high : 0.0f;
    return {hue, saturation, high};
}

glm::vec3 hsvToRgb(glm::vec3 hsv) {
    const float hue = (hsv.x - std::floor(hsv.x)) * 6.0f;
    const float chroma = hsv.z * hsv.y;
    const float x = chroma * (1.0f - std::abs(std::fmod(hue, 2.0f) - 1.0f));
    glm::vec3 rgb(0.0f);
    if (hue < 1.0f) rgb = {chroma, x, 0.0f};
    else if (hue < 2.0f) rgb = {x, chroma, 0.0f};
    else if (hue < 3.0f) rgb = {0.0f, chroma, x};
    else if (hue < 4.0f) rgb = {0.0f, x, chroma};
    else if (hue < 5.0f) rgb = {x, 0.0f, chroma};
    else rgb = {chroma, 0.0f, x};
    return rgb + glm::vec3(hsv.z - chroma);
}

std::string toHex(Color linear, bool alpha) {
    const glm::vec3 srgb = srgbOf(linear);
    const std::array<float, 4> channels = {srgb.r, srgb.g, srgb.b, std::clamp(linear.a, 0.0f, 1.0f)};
    static constexpr char DIGITS[] = "0123456789ABCDEF";
    std::string text;
    for (std::size_t i = 0; i < (alpha ? 4u : 3u); ++i) {
        const int byte = static_cast<int>(std::lround(channels[i] * 255.0f));
        text += DIGITS[(byte >> 4) & 0xF];
        text += DIGITS[byte & 0xF];
    }
    return text;
}

std::optional<Color> fromHex(std::string_view text) {
    while (!text.empty() && (text.front() == ' ' || text.front() == '#')) text.remove_prefix(1);
    while (!text.empty() && text.back() == ' ') text.remove_suffix(1);
    if (text.size() != 6 && text.size() != 8) return std::nullopt;
    std::uint32_t value = 0;
    for (const char c : text) {
        const int digit = hexDigit(c);
        if (digit < 0) return std::nullopt;
        value = (value << 4) | static_cast<std::uint32_t>(digit);
    }
    if (text.size() == 6) value = (value << 8) | 0xFFu;
    return Color::hex(value);
}

void ColorBlock::construct(const Args& args) {
    color_ = args.color_;
    showAlpha_ = args.showAlpha_;
    size_ = args.size_;
    opensPicker_ = args.opensPicker_;
    useAlpha_ = args.useAlpha_;
    style_ = args.style_;
    onChanged_ = args.onColorChanged_;
    onClicked_ = args.onClicked_;
}

void ColorBlock::change(Color color) {
    if (!color_.bound()) color_.set(color);
    if (onChanged_) onChanged_(color);
}

bool ColorBlock::isPickerOpen() const { return picker_ && app_ != nullptr && app_->isPopupOpen(picker_.get()); }

void ColorBlock::openPicker() {
    Application* app = Application::current();
    if (app == nullptr || isPickerOpen()) return;
    const std::weak_ptr<Widget> self = weak_from_this();
    const auto block = [self]() -> ColorBlock* {
        std::shared_ptr<Widget> alive = self.lock();
        return alive ? static_cast<ColorBlock*>(alive.get()) : nullptr;
    };
    const Color initial = color();
    picker_ = make<ColorPicker>()
                      .color([block, initial] {
                          const ColorBlock* owner = block();
                          return owner != nullptr ? owner->color() : initial;
                      })
                      .useAlpha(useAlpha_)
                      .style(style_)
                      .onColorChanged([block](Color color) {
                          if (ColorBlock* owner = block()) owner->change(color);
                      });
    PopupOptions options;
    options.placement = Placement::Below;
    options.owner = self;
    const Widget* raw = picker_.get();
    options.onDismissed = [block, raw] {
        ColorBlock* owner = block();
        if (owner != nullptr && owner->picker_.get() == raw) owner->picker_.reset();
    };
    const Rect anchor = app->grid().geometryOf(this).value_or(Geometry{}).rect();
    app_ = app;
    app->pushPopup(picker_, anchor, std::move(options));
}

Reply ColorBlock::onMouseDown(const Geometry&, const PointerEvent& event) {
    if (event.button != cinder::platform::buttons::LEFT) return Reply::unhandled();
    if (onClicked_) return onClicked_();
    if (!opensPicker_) return Reply::unhandled();
    if (isPickerOpen()) {
        app_->dismissPopup(picker_.get());
    } else {
        openPicker();
    }
    return Reply::handled();
}

int ColorBlock::onPaint(const PaintArgs&, const Geometry& geometry, ElementList& list, int layer,
                        const PaintStyle& style, bool enabled) const {
    const ColorPickerStyle& look = Application::get().theme().get<ColorPickerStyle>(style_);
    const Rect rect = geometry.rect();
    const Color value = color();
    const Color opaque{value.r, value.g, value.b, 1.0f};
    PaintStyle tinted = style;
    if (!enabled) tinted.tint.a *= 0.5f;
    if (showAlpha_ && useAlpha_ && value.a < 1.0f) {
        const float middle = std::round((rect.min.x + rect.max.x) * 0.5f);
        list.box(layer, Rect{rect.min, {middle, rect.max.y}}, BoxStyle{tinted.apply(opaque)});
        const Rect right{{middle, rect.min.y}, rect.max};
        paintChecker(list, layer, right, look, tinted);
        list.box(layer + 1, right, BoxStyle{tinted.apply(value)});
    } else {
        list.box(layer, rect, BoxStyle{tinted.apply(opaque)});
    }
    look.swatchBorder.paint(list, layer + 2, rect, tinted, geometry.scale);
    return layer + 2;
}

void ColorPicker::construct(const Args& args) {
    color_ = args.color_;
    original_ = color_.get();
    useAlpha_ = args.useAlpha_;
    style_ = args.style_;
    onChanged_ = args.onColorChanged_;

    auto areas = make<HorizontalBox>()
            + HorizontalBox::slot().autoWidth()[std::make_shared<PickerArea>(*this, PickerArea::Kind::SaturationValue)]
            + HorizontalBox::slot().autoWidth().padding(Margin(8.0f, 0.0f, 0.0f, 0.0f))
                  [std::make_shared<PickerArea>(*this, PickerArea::Kind::Hue)];
    if (useAlpha_) {
        areas + HorizontalBox::slot().autoWidth().padding(Margin(8.0f, 0.0f, 0.0f, 0.0f))
                        [std::make_shared<PickerArea>(*this, PickerArea::Kind::Alpha)];
    }

    auto swatches = make<HorizontalBox>()
            + HorizontalBox::slot().fill(1.0f)
                  [make<ColorBlock>().color(original_).useAlpha(useAlpha_).style(style_).size(glm::vec2(40.0f, 22.0f))
                       .toolTipText("Revert to the colour before the picker opened")
                       .onClicked([this] {
                           setColor(original_);
                           return Reply::handled();
                       })]
            + HorizontalBox::slot().fill(1.0f)
                  [make<ColorBlock>().color([this] { return color(); }).useAlpha(useAlpha_).style(style_)
                       .size(glm::vec2(40.0f, 22.0f))];

    const int components = useAlpha_ ? 4 : 3;
    auto channels = make<VectorInputBox>()
            .components(components)
            .accents({"Color.AxisX", "Color.AxisY", "Color.AxisZ", "Color.AxisW"})
            .value([this](int index) {
                const Color value = color();
                const std::array<float, 4> parts = {value.r, value.g, value.b, value.a};
                return static_cast<double>(parts[static_cast<std::size_t>(index)]);
            })
            .minValue(0.0)
            .step(0.005)
            .onComponentChanged([this](int index, double value) {
                Color next = color();
                const auto channel = static_cast<float>(value);
                if (index == 0) next.r = channel;
                else if (index == 1) next.g = channel;
                else if (index == 2) next.b = channel;
                else next.a = std::min(channel, 1.0f);
                setColor(next);
            });

    auto hex = make<HorizontalBox>()
            + HorizontalBox::slot().autoWidth().vAlign(VAlign::Center).padding(Margin(0.0f, 0.0f, 8.0f, 0.0f))
                  [make<Label>().text("Hex sRGB").textStyle("Label.Small")]
            + HorizontalBox::slot().fill(1.0f)
                  [make<TextBox>()
                       .text([this] { return toHex(color(), useAlpha_); })
                       .font(FontInfo{cinder::text::FontStyle::Mono, 12.0f})
                       .selectAllOnFocus(true)
                       .onTextCommitted([this](const std::string& text, TextCommit how) {
                           if (how == TextCommit::Cleared) return;
                           if (std::optional<Color> parsed = fromHex(text)) {
                               if (!useAlpha_) parsed->a = color().a;
                               setColor(*parsed);
                           }
                       })];

    childSlot_.widget = make<Border>()
            .brush([this] { return Application::get().theme().get<ColorPickerStyle>(style_).background; })
            .padding(Margin(10.0f))
            [make<VerticalBox>()
             + VerticalBox::slot().autoHeight()[areas]
             + VerticalBox::slot().autoHeight().padding(Margin(0.0f, 10.0f, 0.0f, 0.0f))[swatches]
             + VerticalBox::slot().autoHeight().padding(Margin(0.0f, 8.0f, 0.0f, 0.0f))[channels]
             + VerticalBox::slot().autoHeight().padding(Margin(0.0f, 8.0f, 0.0f, 0.0f))[hex]];
}

void ColorPicker::setColor(Color linear) {
    if (!color_.bound()) color_.set(linear);
    if (onChanged_) onChanged_(linear);
}

glm::vec3 ColorPicker::hsv() const {
    const Color current = color();
    if (hsvFor_ && hsvFor_->r == current.r && hsvFor_->g == current.g && hsvFor_->b == current.b) return hsv_;
    const glm::vec3 next = rgbToHsv(srgbOf(current));
    if (hsvFor_ && next.z > 0.0f && next.y > 0.0f) hsv_ = next;
    else if (hsvFor_ && next.z > 0.0f) hsv_ = {hsv_.x, next.y, next.z};
    else if (hsvFor_) hsv_ = {hsv_.x, hsv_.y, next.z};
    else hsv_ = next;
    hsvFor_ = current;
    return hsv_;
}

void ColorPicker::setHsv(glm::vec3 hsv) {
    hsv_ = glm::clamp(hsv, glm::vec3(0.0f), glm::vec3(1.0f));
    const Color next = linearFromSrgb(hsvToRgb(hsv_), color().a);
    hsvFor_ = next;
    setColor(next);
}

void ColorPicker::setAlpha(float alpha) {
    Color next = color();
    next.a = std::clamp(alpha, 0.0f, 1.0f);
    setColor(next);
}

}
