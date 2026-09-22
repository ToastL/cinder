#include "ui/widgets/SpinBox.hpp"

#include "ui/core/ElementList.hpp"
#include "ui/framework/Application.hpp"
#include "ui/widgets/TextField.hpp"

#include <glm/geometric.hpp>

#include <algorithm>
#include <charconv>
#include <cmath>
#include <system_error>

namespace cinder::ui {

namespace {

constexpr int MAX_FACTORS = 256;

class Expression {
public:
    explicit Expression(std::string_view text) : text_(text) {}

    std::optional<double> parse() {
        std::optional<double> value = sum();
        skip();
        if (!value || at_ != text_.size()) return std::nullopt;
        return value;
    }

private:
    void skip() {
        while (at_ < text_.size() && (text_[at_] == ' ' || text_[at_] == '\t')) ++at_;
    }

    bool take(char c) {
        skip();
        if (at_ < text_.size() && text_[at_] == c) {
            ++at_;
            return true;
        }
        return false;
    }

    std::optional<double> sum() {
        std::optional<double> left = product();
        while (left) {
            if (take('+')) {
                const std::optional<double> right = product();
                if (!right) return std::nullopt;
                *left += *right;
            } else if (take('-')) {
                const std::optional<double> right = product();
                if (!right) return std::nullopt;
                *left -= *right;
            } else {
                break;
            }
        }
        return left;
    }

    std::optional<double> product() {
        std::optional<double> left = factor();
        while (left) {
            if (take('*')) {
                const std::optional<double> right = factor();
                if (!right) return std::nullopt;
                *left *= *right;
            } else if (take('/')) {
                const std::optional<double> right = factor();
                if (!right) return std::nullopt;
                *left /= *right;
            } else {
                break;
            }
        }
        return left;
    }

    std::optional<double> factor() {
        if (++factors_ > MAX_FACTORS) return std::nullopt;
        if (take('-')) {
            std::optional<double> value = factor();
            if (value) *value = -*value;
            return value;
        }
        if (take('+')) return factor();
        if (take('(')) {
            const std::optional<double> value = sum();
            if (!value || !take(')')) return std::nullopt;
            return value;
        }
        skip();
        double value = 0.0;
        const char* begin = text_.data() + at_;
        const auto [end, error] = std::from_chars(begin, text_.data() + text_.size(), value);
        if (error != std::errc() || end == begin) return std::nullopt;
        at_ += static_cast<std::size_t>(end - begin);
        return value;
    }

    std::string_view text_;
    std::size_t at_ = 0;
    int factors_ = 0;
};

std::string_view trimmed(std::string_view text) {
    while (!text.empty() && (text.front() == ' ' || text.front() == '\t')) text.remove_prefix(1);
    while (!text.empty() && (text.back() == ' ' || text.back() == '\t')) text.remove_suffix(1);
    return text;
}

}

std::optional<double> evaluateNumber(std::string_view text) {
    text = trimmed(text);
    if (text.empty()) return std::nullopt;
    const std::optional<double> value = Expression(text).parse();
    if (!value || !std::isfinite(*value)) return std::nullopt;
    return value;
}

std::string formatNumber(double value, int fractionalDigits, bool integral) {
    char buffer[128];
    if (integral || fractionalDigits <= 0) {
        const auto [end, error] = std::to_chars(buffer, buffer + sizeof(buffer), static_cast<long long>(std::llround(value)));
        return error == std::errc() ? std::string(buffer, end) : std::string("0");
    }
    const auto [end, error] = std::to_chars(buffer, buffer + sizeof(buffer), value, std::chars_format::fixed, fractionalDigits);
    if (error != std::errc()) return exactNumber(value, false);
    std::string text(buffer, end);
    const std::size_t point = text.find('.');
    if (point != std::string::npos) {
        std::size_t last = text.size();
        while (last > point + 2 && text[last - 1] == '0') --last;
        text.resize(last);
    }
    if (text == "-0.0") text = "0.0";
    return text;
}

std::string exactNumber(double value, bool integral) {
    char buffer[128];
    if (integral) {
        const auto [end, error] = std::to_chars(buffer, buffer + sizeof(buffer), static_cast<long long>(std::llround(value)));
        return error == std::errc() ? std::string(buffer, end) : std::string("0");
    }
    const auto narrow = static_cast<float>(value);
    const bool single = std::isfinite(narrow) && static_cast<double>(narrow) == value;
    const auto [end, error] = single ? std::to_chars(buffer, buffer + sizeof(buffer), narrow)
                                     : std::to_chars(buffer, buffer + sizeof(buffer), value);
    if (error != std::errc()) return "0";
    std::string text(buffer, end);
    if (text == "-0") text = "0";
    return text;
}

void SpinBox::construct(const Args& args) {
    value_ = args.value_;
    min_ = args.minValue_;
    max_ = args.maxValue_;
    sliderMin_ = args.minSliderValue_;
    sliderMax_ = args.maxSliderValue_;
    step_ = args.step_;
    integral_ = args.integral_;
    digits_ = args.fractionalDigits_;
    units_ = args.units_;
    accent_ = args.accent_;
    style_ = args.style_;
    textStyle_ = args.textStyle_;
    minWidth_ = args.minDesiredWidth_;
    onChanged_ = args.onValueChanged_;
    onCommitted_ = args.onValueCommitted_;
    onBegin_ = args.onBeginSliderMovement_;
    onEnd_ = args.onEndSliderMovement_;
    field_ = make<TextField>()
                     .style(textStyle_)
                     .selectAllOnFocus(true)
                     .onTextCommitted([this](const std::string& text, TextCommit how) { committed(text, how); });
}

Widget* SpinBox::childAt(int index) const { return index == 0 ? field_.get() : nullptr; }

std::string SpinBox::displayText() const { return formatNumber(value(), digits_, integral_) + units_; }

std::optional<double> SpinBox::sliderMin() const { return sliderMin_ ? sliderMin_ : min_; }
std::optional<double> SpinBox::sliderMax() const { return sliderMax_ ? sliderMax_ : max_; }

double SpinBox::constrain(double value) const {
    if (min_) value = std::max(value, *min_);
    if (max_) value = std::min(value, *max_);
    if (integral_) value = std::round(value);
    return value;
}

void SpinBox::write(double value) {
    if (!value_.bound()) value_.set(value);
    if (onChanged_) onChanged_(value);
}

void SpinBox::beginEditing() {
    editText_ = exactNumber(value(), integral_);
    field_->setText(editText_);
    editing_ = true;
    if (Application* app = Application::current()) app->setFocus(field_, FocusCause::SetDirectly);
}

void SpinBox::committed(const std::string& text, TextCommit how) {
    editing_ = false;
    if (how == TextCommit::Cleared || text == editText_) return;
    std::string_view typed = trimmed(text);
    if (!units_.empty() && typed.size() >= units_.size() && typed.substr(typed.size() - units_.size()) == units_) {
        typed.remove_suffix(units_.size());
    }
    const std::optional<double> parsed = evaluateNumber(typed);
    if (!parsed) return;
    const double value = constrain(*parsed);
    write(value);
    if (onCommitted_) onCommitted_(value, how);
}

void SpinBox::tick(const Geometry&, double, float) {
    if (editing_ && !field_->hasFocus()) editing_ = false;
}

void SpinBox::arrangeChildren(const Geometry& geometry, ArrangedChildren& out) const {
    if (!editing_) return;
    const SpinBoxStyle& look = Application::get().theme().get<SpinBoxStyle>(style_);
    const float accent = accent_.empty() ? 0.0f : look.accentWidth;
    const glm::vec2 inset = look.padding.topLeft() + glm::vec2(accent, 0.0f);
    const glm::vec2 size = glm::max(glm::vec2(0.0f), geometry.size - look.padding.total() - glm::vec2(accent, 0.0f));
    out.push_back({field_, geometry.child(inset, size)});
}

Reply SpinBox::onMouseDown(const Geometry&, const PointerEvent& event) {
    if (event.button != cinder::platform::buttons::LEFT) return Reply::unhandled();
    if (editing_) return Reply::handled();
    pressed_ = true;
    dragging_ = false;
    pressAt_ = event.position;
    dragValue_ = value();
    return Reply::handled().captureMouse(shared_from_this());
}

Reply SpinBox::onMouseMove(const Geometry&, const PointerEvent& event) {
    if (!pressed_) return Reply::unhandled();
    if (!dragging_) {
        if (glm::distance(event.position, pressAt_) <= Application::DRAG_THRESHOLD) return Reply::handled();
        dragging_ = true;
        if (onBegin_) onBegin_();
        return Reply::handled();
    }
    const double speed = step_ * (event.shift() ? FAST : (event.alt() ? SLOW : 1.0));
    dragValue_ += static_cast<double>(event.delta.x) * speed;
    if (min_) dragValue_ = std::max(dragValue_, *min_);
    if (max_) dragValue_ = std::min(dragValue_, *max_);
    const double next = integral_ ? std::round(dragValue_) : dragValue_;
    if (next != value()) write(next);
    return Reply::handled();
}

Reply SpinBox::onMouseUp(const Geometry&, const PointerEvent& event) {
    if (!pressed_ || event.button != cinder::platform::buttons::LEFT) return Reply::unhandled();
    pressed_ = false;
    if (dragging_) {
        dragging_ = false;
        if (onEnd_) onEnd_(value());
        if (onCommitted_) onCommitted_(value(), TextCommit::Enter);
        return Reply::handled().releaseMouseCapture();
    }
    editText_ = exactNumber(value(), integral_);
    field_->setText(editText_);
    editing_ = true;
    return Reply::handled().releaseMouseCapture().setFocus(field_);
}

void SpinBox::onMouseCaptureLost() {
    pressed_ = false;
    if (!dragging_) return;
    dragging_ = false;
    if (onEnd_) onEnd_(value());
    if (onCommitted_) onCommitted_(value(), TextCommit::FocusLost);
}

std::optional<cinder::platform::CursorShape> SpinBox::onCursorQuery(const Geometry&, const PointerEvent&) const {
    if (editing_) return std::nullopt;
    return cinder::platform::CursorShape::ResizeHorizontal;
}

glm::vec2 SpinBox::computeDesiredSize(float) const {
    Application& app = Application::get();
    const SpinBoxStyle& look = app.theme().get<SpinBoxStyle>(style_);
    const TextFieldStyle& text = app.theme().get<TextFieldStyle>(textStyle_);
    run_.shape(app.fonts(), displayText(), text.font);
    glm::vec2 content = run_.size();
    content.y = std::max(content.y, field_->desiredSize().y);
    glm::vec2 size = content + look.padding.total();
    if (!accent_.empty()) size.x += look.accentWidth;
    size.x = std::max(size.x, minWidth_.get());
    return size;
}

int SpinBox::onPaint(const PaintArgs& args, const Geometry& geometry, ElementList& list, int layer,
                     const PaintStyle& style, bool enabled) const {
    Application& app = Application::get();
    const SpinBoxStyle& look = app.theme().get<SpinBoxStyle>(style_);
    const TextFieldStyle& text = app.theme().get<TextFieldStyle>(textStyle_);
    const Rect rect = geometry.rect();
    const bool active = dragging_ || isHovered();

    const Brush& brush = editing_ ? look.editing : (dragging_ ? look.active : (isHovered() ? look.hovered : look.normal));
    brush.paint(list, layer, rect, style, geometry.scale);

    const std::optional<double> low = sliderMin();
    const std::optional<double> high = sliderMax();
    if (!editing_ && low && high && *high > *low) {
        const auto fraction = static_cast<float>(std::clamp((value() - *low) / (*high - *low), 0.0, 1.0));
        (active ? look.fillHovered : look.fill)
                .paint(list, layer + 1, geometry.localRect({0.0f, 0.0f}, {geometry.size.x * fraction, geometry.size.y}),
                       style, geometry.scale);
    }

    float left = look.padding.left;
    if (!accent_.empty()) {
        const float radius = brush.radii.x * geometry.scale;
        Color color = style.apply(app.theme().color(accent_));
        if (!enabled) color.a *= 0.4f;
        list.box(layer + 1, geometry.localRect({0.0f, 0.0f}, {look.accentWidth, geometry.size.y}),
                 BoxStyle{color, Color::transparent(), 0.0f, glm::vec4(radius, 0.0f, 0.0f, radius)});
        left += look.accentWidth;
    }

    if (editing_) return paintChildren(args, geometry, list, layer + 2, style, enabled);

    run_.shape(app.fonts(), displayText(), text.font);
    Color color = style.apply(text.color);
    if (!enabled) color.a *= 0.4f;
    const float top = std::round((geometry.size.y - run_.size().y) * 0.5f);
    list.pushClip(rect);
    run_.paint(list, layer + 2, geometry.absolute({left, top}), geometry.scale, color, app.atlas());
    list.popClip();
    return layer + 2;
}

}
