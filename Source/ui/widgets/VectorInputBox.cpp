#include "ui/widgets/VectorInputBox.hpp"

#include "ui/widgets/SpinBox.hpp"

#include <algorithm>

namespace cinder::ui {

void VectorInputBox::construct(const Args& args) {
    static const std::vector<std::string> AXES = {"Color.AxisX", "Color.AxisY", "Color.AxisZ", "Color.AxisW"};
    const std::vector<std::string>& accents = args.accents_.empty() ? AXES : args.accents_;
    const int count = std::clamp(args.components_, 1, 4);
    const Getter getter = args.value_;

    for (int i = 0; i < count; ++i) {
        const std::string accent = args.colorAxes_ && static_cast<std::size_t>(i) < accents.size()
                ? accents[static_cast<std::size_t>(i)]
                : std::string();
        const OnComponentChanged changed = args.onComponentChanged_;
        const OnComponentCommitted committed = args.onComponentCommitted_;
        const OnVoid end = args.onEndSliderMovement_;
        std::shared_ptr<SpinBox> box = make<SpinBox>()
                .value([getter, i] { return getter ? getter(i) : 0.0; })
                .minValue(args.minValue_)
                .maxValue(args.maxValue_)
                .step(args.step_)
                .fractionalDigits(args.fractionalDigits_)
                .units(args.units_)
                .accent(accent)
                .minDesiredWidth(0.0f)
                .onValueChanged([changed, i](double value) {
                    if (changed) changed(i, value);
                })
                .onValueCommitted([committed, i](double value, TextCommit how) {
                    if (committed) committed(i, value, how);
                })
                .onBeginSliderMovement(args.onBeginSliderMovement_)
                .onEndSliderMovement([end](double) {
                    if (end) end();
                });
        boxes_.push_back(box);
        addSlot(slot().fill(1.0f).padding(Margin(i == 0 ? 0.0f : args.spacing_, 0.0f, 0.0f, 0.0f))[box]);
    }
}

}
