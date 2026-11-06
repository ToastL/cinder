#include "dev/panels/Dialog.hpp"

#include "ui/framework/Application.hpp"
#include "ui/widgets/Border.hpp"
#include "ui/widgets/BoxPanel.hpp"
#include "ui/widgets/Button.hpp"
#include "ui/widgets/Label.hpp"
#include "ui/widgets/SizeBox.hpp"

namespace cinder::dev::panels {

using namespace cinder::ui;
namespace keys = cinder::platform::keys;

void Dialog::construct(const Args& args) {
    onCancel_ = args.onCancel_;
    auto buttons = make<HorizontalBox>() + HorizontalBox::slot().fill(1.0f)[make<Spacer>()];
    for (const DialogChoice& choice : args.choices_) {
        if (choice.primary) primary_ = choice.action;
        std::function<void()> action = choice.action;
        buttons + HorizontalBox::slot().autoWidth().padding(Margin(6.0f, 0.0f, 0.0f, 0.0f))
                          [make<Button>()
                                   .text(choice.label)
                                   .buttonStyle(choice.primary ? "Button.Primary" : "Button")
                                   .onClicked([action] {
                                       if (action) action();
                                       return Reply::handled();
                                   })];
    }

    std::shared_ptr<Widget> card = make<Border>().brush([] { return Application::get().theme().get<Brush>("Brush.Dialog"); })
            .padding(Margin(18.0f, 14.0f))
            [make<SizeBox>().minDesiredWidth(360.0f)
                 [make<VerticalBox>()
                  + VerticalBox::slot().autoHeight().padding(Margin(0.0f, 0.0f, 0.0f, 8.0f))
                        [make<Label>().text(args.title_).textStyle("Label.Header")]
                  + VerticalBox::slot().autoHeight().padding(Margin(0.0f, 0.0f, 0.0f, 16.0f))[make<Label>().text(args.message_)]
                  + VerticalBox::slot().autoHeight()[buttons]]];

    childSlot_.widget = make<Border>()
            .brush([] { return Application::get().theme().get<Brush>("Brush.Dim"); })
            .padding(Margin(0.0f))
            .hAlign(HAlign::Center)
            .vAlign(VAlign::Center)
            .onMouseDown([](const Geometry&, const PointerEvent&) { return Reply::handled(); })[card];
}

Reply Dialog::onKeyDown(const Geometry&, const KeyEvent& event) {
    if (event.key == keys::ESCAPE) {
        if (onCancel_) onCancel_();
        return Reply::handled();
    }
    if ((event.key == keys::ENTER || event.key == keys::KEYPAD_ENTER) && primary_) {
        primary_();
        return Reply::handled();
    }
    return Reply::unhandled();
}

Reply Dialog::onMouseDown(const Geometry&, const PointerEvent&) {
    return Reply::handled().setFocus(shared_from_this());
}

}
