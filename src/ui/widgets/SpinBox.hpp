#pragma once

#include "ui/core/Args.hpp"
#include "ui/core/Delegates.hpp"
#include "ui/core/TextRun.hpp"
#include "ui/core/Widget.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace cinder::ui {

class TextField;

std::optional<double> evaluateNumber(std::string_view text);
std::string formatNumber(double value, int fractionalDigits, bool integral);
std::string exactNumber(double value, bool integral);

class SpinBox : public Widget {
public:
    using OnValueChanged = std::function<void(double)>;
    using OnValueCommitted = std::function<void(double, TextCommit)>;

    struct Args : ::cinder::ui::Args<Args, SpinBox> {
        UI_ATTR(double, value)
        UI_ARG(std::optional<double>, minValue)
        UI_ARG(std::optional<double>, maxValue)
        UI_ARG(std::optional<double>, minSliderValue)
        UI_ARG(std::optional<double>, maxSliderValue)
        UI_ARG(double, step, 0.1)
        UI_ARG(bool, integral)
        UI_ARG(int, fractionalDigits, 3)
        UI_ARG(std::string, units)
        UI_ARG(std::string, accent)
        UI_ARG(std::string, style, "SpinBox")
        UI_ARG(std::string, textStyle, "TextField")
        UI_ATTR(float, minDesiredWidth, 48.0f)
        UI_EVENT(OnValueChanged, onValueChanged)
        UI_EVENT(OnValueCommitted, onValueCommitted)
        UI_EVENT(OnVoid, onBeginSliderMovement)
        UI_EVENT(OnValueChanged, onEndSliderMovement)
    };

    static constexpr double FAST = 10.0;
    static constexpr double SLOW = 0.1;

    void construct(const Args& args);

    double value() const { return value_.get(); }
    bool isEditing() const { return editing_; }
    bool isDragging() const { return dragging_; }
    const std::shared_ptr<TextField>& field() const { return field_; }
    std::string displayText() const;
    void beginEditing();

    int childCount() const override { return 1; }
    Widget* childAt(int index) const override;
    void arrangeChildren(const Geometry& geometry, ArrangedChildren& out) const override;

    Reply onMouseDown(const Geometry& geometry, const PointerEvent& event) override;
    Reply onMouseMove(const Geometry& geometry, const PointerEvent& event) override;
    Reply onMouseUp(const Geometry& geometry, const PointerEvent& event) override;
    void onMouseCaptureLost() override;
    std::optional<cinder::platform::CursorShape> onCursorQuery(const Geometry& geometry,
                                                               const PointerEvent& event) const override;
    void tick(const Geometry& geometry, double time, float deltaTime) override;

protected:
    glm::vec2 computeDesiredSize(float layoutScale) const override;
    int onPaint(const PaintArgs& args, const Geometry& geometry, ElementList& list, int layer,
                const PaintStyle& style, bool enabled) const override;

private:
    double constrain(double value) const;
    void write(double value);
    void committed(const std::string& text, TextCommit how);
    std::optional<double> sliderMin() const;
    std::optional<double> sliderMax() const;

    Attribute<double> value_;
    std::optional<double> min_;
    std::optional<double> max_;
    std::optional<double> sliderMin_;
    std::optional<double> sliderMax_;
    double step_ = 0.1;
    bool integral_ = false;
    int digits_ = 3;
    std::string units_;
    std::string accent_;
    std::string style_;
    std::string textStyle_;
    Attribute<float> minWidth_{48.0f};
    OnValueChanged onChanged_;
    OnValueCommitted onCommitted_;
    OnVoid onBegin_;
    OnValueChanged onEnd_;

    std::shared_ptr<TextField> field_;
    std::string editText_;
    double dragValue_ = 0.0;
    glm::vec2 pressAt_{0.0f};
    bool pressed_ = false;
    bool dragging_ = false;
    bool editing_ = false;
    mutable TextRun run_;
};

}
