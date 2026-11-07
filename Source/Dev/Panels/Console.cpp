#include "Dev/Panels/Console.hpp"

#include "Dev/Panels/EditorStyle.hpp"
#include "UI/Framework/Application.hpp"
#include "UI/Widgets/Border.hpp"
#include "UI/Widgets/BoxPanel.hpp"
#include "UI/Widgets/Button.hpp"
#include "UI/Widgets/Label.hpp"
#include "UI/Widgets/ScrollBox.hpp"

#include <utility>

namespace cinder::Dev::Panels {

using namespace cinder::UI;

Console::Console(Application& app) : app_(app) {
    build();
    cinder::Platform::setLogSink(
            [this](cinder::Platform::LogLevel level, std::string_view text) { push(level, text); });
}

Console::~Console() { cinder::Platform::setLogSink(nullptr); }

void Console::build() {
    widget_ = make<VerticalBox>()
        + VerticalBox::slot().autoHeight()
              [titleBar(make<HorizontalBox>()
                    + HorizontalBox::slot().autoWidth().padding(Margin(0.0f, 0.0f, 6.0f, 0.0f))
                          [make<Button>().text("Clear").buttonStyle("Button.Small").onClicked([this] {
                              clear();
                              return Reply::handled();
                          })]
                    + HorizontalBox::slot().autoWidth().vAlign(VAlign::Center)
                          [make<Label>()
                                   .text([this] { return std::to_string(count_) + (count_ == 1 ? " line" : " lines"); })
                                   .textStyle("Label.Small")
                                   .colorAndOpacity(themeColor("Color.ForegroundDim"))])]
        + VerticalBox::slot().fill(1.0f)
              [make<Border>()
                       .brush([] { return Application::get().theme().get<Brush>("Brush.Recessed"); })
                       .padding(Margin(4.0f, 2.0f))[make<ScrollBox>().assign(scroll_)]];
}

void Console::push(cinder::Platform::LogLevel level, std::string_view text) {
    pending_.push_back(Line{level, std::string(text)});
}

void Console::clear() {
    pending_.clear();
    scroll_->clearChildren();
    count_ = 0;
}

void Console::update() {
    if (pending_.empty()) return;
    const bool follow = scroll_->atEnd();
    std::vector<Line> lines = std::move(pending_);
    pending_.clear();

    for (Line& line : lines) {
        const bool error = line.level == cinder::Platform::LogLevel::Error;
        std::optional<Attribute<Color>> color;
        if (error) color = Attribute<Color>([] { return Application::get().theme().color("Color.ConsoleError"); });
        scroll_->addSlot(ScrollBox::slot().padding(Margin(2.0f, 0.0f))
                                 [make<Label>().text(std::move(line.text)).textStyle("Label.Console").colorAndOpacity(color)]);
        ++count_;
    }
    if (count_ > MAX_LINES) {
        scroll_->removeFront(count_ - MAX_LINES);
        count_ = MAX_LINES;
    }
    if (follow) scroll_->scrollToEnd();
}

}
