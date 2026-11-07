#include "UI/Widgets/ComboBox.hpp"

#include "UI/Widgets/BoxPanel.hpp"
#include "UI/Widgets/Button.hpp"
#include "UI/Widgets/ExpandableArea.hpp"
#include "UI/Widgets/Label.hpp"

namespace cinder::UI {

void ComboButton::construct(const Args& args) {
    auto row = make<HorizontalBox>() + HorizontalBox::slot().fill(1.0f).vAlign(VAlign::Center)[args.buttonContent_];
    if (args.hasDownArrow_) {
        row + HorizontalBox::slot().autoWidth().vAlign(VAlign::Center).padding(Margin(6.0f, 0.0f, 0.0f, 0.0f))
                      [make<ExpanderArrow>().expanded(true).size(9.0f)];
    }
    button_ = make<Button>().buttonStyle(args.buttonStyle_).hAlign(HAlign::Fill).onClicked([this] {
        anchor_->toggle();
        return Reply::handled();
    })[row];
    anchor_ = make<MenuAnchor>()
                      .onGetMenuContent(args.onGetMenuContent_)
                      .placement(args.placement_)
                      .matchWidth(true)
                      .onMenuOpenChanged(args.onMenuOpenChanged_)[button_];
    childSlot_.widget = anchor_;
}

void ComboBox::construct(const Args& args) {
    options_ = args.options_;
    selected_ = args.selectedOption_;
    onChanged_ = args.onSelectionChanged_;
    labelOf_ = args.labelOf_;
    button_ = make<ComboButton>()
                      .buttonStyle(args.buttonStyle_)
                      .onGetMenuContent([this] { return buildMenu(); })
                      [make<Label>().text([this] { return labelFor(selected()); })];
    childSlot_.widget = button_;
}

void ComboBox::select(const std::string& option) {
    if (!selected_.bound()) selected_.set(option);
    if (onChanged_) onChanged_(option);
}

std::shared_ptr<Widget> ComboBox::buildMenu() {
    MenuBuilder builder;
    const std::string current = selected();
    int chosen = -1;
    const std::vector<std::string> all = options();
    const std::weak_ptr<Widget> self = weak_from_this();
    for (std::size_t i = 0; i < all.size(); ++i) {
        const std::string option = all[i];
        if (option == current) chosen = static_cast<int>(i);
        builder.check(labelFor(option),
                      [self, option] {
                          if (std::shared_ptr<Widget> alive = self.lock()) static_cast<ComboBox&>(*alive).select(option);
                      },
                      [self, option] {
                          std::shared_ptr<Widget> alive = self.lock();
                          return alive && static_cast<const ComboBox&>(*alive).selected() == option;
                      });
    }
    std::shared_ptr<Menu> menu = builder.build();
    if (chosen >= 0) menu->highlight(chosen, false);
    return menu;
}

}
