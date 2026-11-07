#include "Dev/Panels/EditorStyle.hpp"

#include "UI/Framework/Application.hpp"
#include "UI/Widgets/Border.hpp"
#include "UI/Widgets/DefaultTheme.hpp"

#include <utility>

namespace cinder::Dev::Panels {

using cinder::UI::Brush;
using cinder::UI::ButtonStyle;
using cinder::UI::Color;
using cinder::UI::LabelStyle;
using cinder::UI::Margin;

cinder::UI::Theme editorTheme() {
    cinder::UI::Theme theme = cinder::UI::defaultTheme();
    const Color primary = theme.color("Color.Primary");

    ButtonStyle toolbar = theme.get<ButtonStyle>("Button.Toolbar");
    ButtonStyle on = toolbar;
    on.normal = Brush::rounded(primary.withAlpha(0.35f), 4.0f, primary, 1.0f);
    on.hovered = Brush::rounded(primary.withAlpha(0.5f), 4.0f, primary, 1.0f);
    on.foreground = theme.color("Color.ForegroundBright");
    theme.set("Button.ToolbarOn", on);

    ButtonStyle small = toolbar;
    small.padding = Margin(6.0f, 1.0f);
    theme.set("Button.Small", small);

    theme.set("Brush.Toolbar", Brush::color(theme.color("Color.Title")));
    theme.set("Brush.TitleBar", Brush::color(theme.color("Color.Header")));
    theme.set("Brush.Dim", Brush::color(Color::hex(0x00000099)));
    theme.set("Brush.Dialog", Brush::rounded(theme.color("Color.Panel"), 6.0f, theme.color("Color.Outline"), 1.0f));
    theme.set("Label.Console", LabelStyle{{cinder::Text::FontStyle::Mono, 12.0f}, theme.color("Color.Foreground")});
    theme.set("Color.ConsoleError", Color::hex(0xFF7366FF));
    return theme;
}

cinder::UI::Attribute<cinder::UI::Color> themeColor(std::string name) {
    return cinder::UI::Attribute<cinder::UI::Color>(
            [name = std::move(name)] { return cinder::UI::Application::get().theme().color(name); });
}

std::shared_ptr<cinder::UI::Widget> titleBar(std::shared_ptr<cinder::UI::Widget> content, cinder::UI::Margin padding) {
    return cinder::UI::make<cinder::UI::Border>()
            .brush([] { return cinder::UI::Application::get().theme().get<cinder::UI::Brush>("Brush.TitleBar"); })
            .padding(padding)[std::move(content)];
}

}
