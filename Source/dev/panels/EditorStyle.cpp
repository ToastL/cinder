#include "dev/panels/EditorStyle.hpp"

#include "ui/framework/Application.hpp"
#include "ui/widgets/Border.hpp"
#include "ui/widgets/DefaultTheme.hpp"

#include <utility>

namespace cinder::dev::panels {

using cinder::ui::Brush;
using cinder::ui::ButtonStyle;
using cinder::ui::Color;
using cinder::ui::LabelStyle;
using cinder::ui::Margin;

cinder::ui::Theme editorTheme() {
    cinder::ui::Theme theme = cinder::ui::defaultTheme();
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
    theme.set("Label.Console", LabelStyle{{cinder::text::FontStyle::Mono, 12.0f}, theme.color("Color.Foreground")});
    theme.set("Color.ConsoleError", Color::hex(0xFF7366FF));
    return theme;
}

cinder::ui::Attribute<cinder::ui::Color> themeColor(std::string name) {
    return cinder::ui::Attribute<cinder::ui::Color>(
            [name = std::move(name)] { return cinder::ui::Application::get().theme().color(name); });
}

std::shared_ptr<cinder::ui::Widget> titleBar(std::shared_ptr<cinder::ui::Widget> content, cinder::ui::Margin padding) {
    return cinder::ui::make<cinder::ui::Border>()
            .brush([] { return cinder::ui::Application::get().theme().get<cinder::ui::Brush>("Brush.TitleBar"); })
            .padding(padding)[std::move(content)];
}

}
