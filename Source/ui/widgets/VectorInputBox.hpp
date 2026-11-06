#pragma once

#include "ui/core/Args.hpp"
#include "ui/core/Delegates.hpp"
#include "ui/widgets/BoxPanel.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace cinder::ui {

class SpinBox;

class VectorInputBox : public HorizontalBox {
public:
    using Getter = std::function<double(int)>;
    using OnComponentChanged = std::function<void(int, double)>;
    using OnComponentCommitted = std::function<void(int, double, TextCommit)>;

    struct Args : ::cinder::ui::Args<Args, VectorInputBox> {
        UI_ARG(int, components, 3)
        UI_EVENT(Getter, value)
        UI_ARG(std::optional<double>, minValue)
        UI_ARG(std::optional<double>, maxValue)
        UI_ARG(double, step, 0.1)
        UI_ARG(int, fractionalDigits, 3)
        UI_ARG(std::string, units)
        UI_ARG(bool, colorAxes, true)
        UI_ARG(std::vector<std::string>, accents)
        UI_ARG(float, spacing, 4.0f)
        UI_EVENT(OnComponentChanged, onComponentChanged)
        UI_EVENT(OnComponentCommitted, onComponentCommitted)
        UI_EVENT(OnVoid, onBeginSliderMovement)
        UI_EVENT(OnVoid, onEndSliderMovement)
    };

    void construct(const Args& args);

    const std::shared_ptr<SpinBox>& component(int index) const { return boxes_[static_cast<std::size_t>(index)]; }
    int components() const { return static_cast<int>(boxes_.size()); }

private:
    std::vector<std::shared_ptr<SpinBox>> boxes_;
};

}
