#include "UI/Widgets/DefaultTheme.hpp"

namespace cinder::UI {

using cinder::Text::FontStyle;

Theme defaultTheme() {
    Theme theme;

    const Color black = Color::hex(0x121212FF);
    const Color title = Color::hex(0x1D1D1DFF);
    const Color background = Color::hex(0x303030FF);
    const Color recessed = Color::hex(0x1E1E1EFF);
    const Color panel = Color::hex(0x2B2B2BFF);
    const Color header = Color::hex(0x3C3C3CFF);
    const Color input = Color::hex(0x272727FF);
    const Color dropdown = Color::hex(0x545454FF);
    const Color outline = Color::hex(0x1A1A1AFF);
    const Color borderShadow = Color::hex(0x101010FF);
    const Color hover = Color::hex(0x656565FF);
    const Color hoverBright = Color::hex(0x7A7A7AFF);
    const Color foreground = Color::hex(0xD7D7D7FF);
    const Color bright = Color::hex(0xFFFFFFFF);
    const Color dim = Color::hex(0x888888FF);
    const Color primary = Color::hex(0x4772B3FF);
    const Color primaryHover = Color::hex(0x5583CBFF);
    const Color primaryPress = Color::hex(0x375A91FF);
    const Color white = Color::hex(0xFFFFFFFF);

    const float r = 4.0f;
    const float rSmall = 2.0f;
    const float rPill = 8.0f;

    theme.set("Color.Black", black);
    theme.set("Color.Title", title);
    theme.set("Color.Background", background);
    theme.set("Color.Recessed", recessed);
    theme.set("Color.Panel", panel);
    theme.set("Color.Header", header);
    theme.set("Color.Input", input);
    theme.set("Color.Dropdown", dropdown);
    theme.set("Color.Outline", outline);
    theme.set("Color.Hover", hover);
    theme.set("Color.Foreground", foreground);
    theme.set("Color.ForegroundBright", bright);
    theme.set("Color.ForegroundDim", dim);
    theme.set("Color.Primary", primary);
    theme.set("Color.Select", primary);
    theme.set("Color.Error", Color::hex(0xE45555FF));
    theme.set("Color.Warning", Color::hex(0xE0A040FF));
    theme.set("Color.Success", Color::hex(0x8BC24AFF));
    theme.set("Color.AxisX", Color::hex(0xE55050FF));
    theme.set("Color.AxisY", Color::hex(0x8BD049FF));
    theme.set("Color.AxisZ", Color::hex(0x3C8EE6FF));
    theme.set("Color.AxisW", Color::hex(0xB0B0B0FF));

    theme.set("Brush.Background", Brush::color(background));
    theme.set("Brush.Recessed", Brush::color(recessed));
    theme.set("Brush.Panel", Brush::color(panel));
    theme.set("Brush.Header", Brush::color(header));
    theme.set("Brush.Title", Brush::color(title));

    theme.set("Label", LabelStyle{{FontStyle::Regular, 12.0f}, foreground});
    theme.set("Label.Small", LabelStyle{{FontStyle::Regular, 10.0f}, foreground});
    theme.set("Label.Bold", LabelStyle{{FontStyle::Bold, 12.0f}, foreground});
    theme.set("Label.Header", LabelStyle{{FontStyle::Bold, 12.0f}, bright});
    theme.set("Label.Mono", LabelStyle{{FontStyle::Mono, 11.0f}, foreground});

    ButtonStyle button;
    button.normal = Brush::rounded(dropdown, r, outline, 1.0f);
    button.hovered = Brush::rounded(hover, r, outline, 1.0f);
    button.pressed = Brush::rounded(recessed, r, outline, 1.0f);
    button.disabled = Brush::rounded(background, r, borderShadow, 1.0f);
    button.padding = Margin(8.0f, 3.0f);
    button.foreground = foreground;
    button.disabledForeground = dim;
    theme.set("Button", button);

    ButtonStyle primaryButton = button;
    primaryButton.normal = Brush::rounded(primary, r, outline, 1.0f);
    primaryButton.hovered = Brush::rounded(primaryHover, r, outline, 1.0f);
    primaryButton.pressed = Brush::rounded(primaryPress, r, outline, 1.0f);
    primaryButton.foreground = white;
    theme.set("Button.Primary", primaryButton);

    ButtonStyle toolbar = button;
    toolbar.normal = Brush::none();
    toolbar.hovered = Brush::rounded(hover, r);
    toolbar.pressed = Brush::rounded(recessed, r);
    toolbar.disabled = Brush::none();
    toolbar.padding = Margin(7.0f, 3.0f);
    theme.set("Button.Toolbar", toolbar);

    CheckBoxStyle check;
    check.box = Brush::rounded(input, rSmall, outline, 1.0f);
    check.hovered = Brush::rounded(hover, rSmall, outline, 1.0f);
    check.checked = Brush::rounded(primary, rSmall, outline, 1.0f);
    check.checkedHovered = Brush::rounded(primaryHover, rSmall, outline, 1.0f);
    check.mark = white;
    check.size = 13.0f;
    theme.set("CheckBox", check);

    TextFieldStyle field;
    field.font = {FontStyle::Regular, 12.0f};
    field.color = foreground;
    field.hint = dim;
    field.selection = primary.withAlpha(0.55f);
    field.caret = bright;
    theme.set("TextField", field);

    TextFieldStyle mono = field;
    mono.font = {FontStyle::Mono, 11.0f};
    theme.set("TextField.Mono", mono);

    TextBoxStyle box;
    box.normal = Brush::rounded(input, r, outline, 1.0f);
    box.hovered = Brush::rounded(input, r, hover, 1.0f);
    box.focused = Brush::rounded(input, r, primary, 1.0f);
    box.disabled = Brush::rounded(background, r, borderShadow, 1.0f);
    box.padding = Margin(6.0f, 3.0f);
    theme.set("TextBox", box);

    ScrollBarStyle scroll;
    scroll.track = Brush::none();
    scroll.thumb = Brush::rounded(Color::hex(0x5A5A5AFF), rPill);
    scroll.thumbHovered = Brush::rounded(Color::hex(0x737373FF), rPill);
    scroll.thumbDragged = Brush::rounded(primary, rPill);
    scroll.thickness = 12.0f;
    theme.set("ScrollBar", scroll);

    SplitterStyle splitter;
    splitter.handle = Brush::color(outline);
    splitter.handleHovered = Brush::color(primary);
    splitter.handleSize = 4.0f;
    theme.set("Splitter", splitter);

    SpinBoxStyle spin;
    spin.normal = Brush::rounded(input, r, outline, 1.0f);
    spin.hovered = Brush::rounded(hover, r, outline, 1.0f);
    spin.active = Brush::rounded(input, r, primary, 1.0f);
    spin.editing = Brush::rounded(input, r, primary, 1.0f);
    spin.fill = Brush::rounded(primary.withAlpha(0.35f), r);
    spin.fillHovered = Brush::rounded(primary.withAlpha(0.5f), r);
    spin.padding = Margin(6.0f, 3.0f);
    spin.accentWidth = 3.0f;
    theme.set("SpinBox", spin);

    ButtonStyle combo = button;
    combo.normal = Brush::rounded(input, r, outline, 1.0f);
    combo.hovered = Brush::rounded(hover, r, outline, 1.0f);
    combo.pressed = Brush::rounded(recessed, r, outline, 1.0f);
    combo.disabled = Brush::rounded(background, r, borderShadow, 1.0f);
    combo.padding = Margin(6.0f, 3.0f);
    theme.set("ComboButton", combo);

    ExpandableAreaStyle area;
    area.header = Brush::rounded(header, rSmall, outline, 1.0f);
    area.headerHovered = Brush::rounded(Color::hex(0x4A4A4AFF), rSmall, outline, 1.0f);
    area.body = Brush::none();
    theme.set("ExpandableArea", area);

    MenuStyle menu;
    menu.background = Brush::rounded(Color::hex(0x252525FF), r, outline, 1.0f);
    menu.highlight = Brush::rounded(primary, rSmall);
    menu.padding = Margin(3.0f);
    menu.entryPadding = Margin(10.0f, 4.0f);
    menu.font = {FontStyle::Regular, 12.0f};
    menu.color = foreground;
    menu.dim = dim;
    menu.separator = Color::hex(0x3C3C3CFF);
    theme.set("Menu", menu);
    theme.set("Menu.Heading", LabelStyle{{FontStyle::Bold, 10.0f}, Color::hex(0x9A9A9AFF)});

    MenuBarStyle bar;
    bar.background = Brush::none();
    bar.item = Brush::none();
    bar.itemHovered = Brush::rounded(hover, r);
    bar.itemOpen = Brush::rounded(primary, r);
    bar.itemPadding = Margin(8.0f, 3.0f);
    bar.font = {FontStyle::Regular, 12.0f};
    bar.color = foreground;
    theme.set("MenuBar", bar);

    ColorPickerStyle picker;
    picker.background = Brush::rounded(panel, r, outline, 1.0f);
    picker.swatchBorder = Brush::rounded(Color::transparent(), rSmall, outline, 1.0f);
    picker.checkerLight = Color::hex(0x606060FF);
    picker.checkerDark = Color::hex(0x404040FF);
    picker.marker = bright;
    picker.checkerSize = 6.0f;
    theme.set("ColorPicker", picker);

    TableViewStyle table;
    table.background = Brush::none();
    table.rowHovered = Brush::color(Color::hex(0x3E3E3EFF));
    table.rowSelected = Brush::color(primary);
    table.rowSelectedInactive = Brush::color(Color::hex(0x455166FF));
    table.dropTarget = primaryHover;
    table.rowPadding = Margin(4.0f, 0.0f);
    table.indent = 14.0f;
    table.rowHeight = 20.0f;
    theme.set("TableView", table);

    const Color tabEdge = Color::hex(0x484848FF);
    const Color tabInactive = Color::hex(0x2A2A2AFF);

    auto topRounded = [](Color fill, float corner, Color border = Color::transparent(), float borderWidth = 0.0f) {
        Brush b;
        b.drawAs = Brush::DrawAs::Box;
        b.fill = fill;
        b.outline = border;
        b.outlineWidth = borderWidth;
        b.radii = glm::vec4(corner, corner, 0.0f, 0.0f);
        return b;
    };

    DockStyle dock;
    dock.background = Brush::rounded(panel, r, tabEdge, 1.0f);
    dock.bar = Brush::color(title);
    dock.tab = topRounded(tabInactive, 6.0f, tabEdge, 1.0f);
    dock.tabHovered = topRounded(hover, 6.0f, tabEdge, 1.0f);
    dock.tabActive = topRounded(panel, 6.0f, tabEdge, 1.0f);
    dock.label = dim;
    dock.labelActive = bright;
    dock.dropFill = primary.withAlpha(0.25f);
    dock.dropOutline = primary;
    dock.tabPadding = Margin(12.0f, 4.0f);
    dock.contentPadding = Margin(1.0f);
    dock.barHeight = 24.0f;
    dock.closeSize = 9.0f;
    theme.set("Dock", dock);

    ToolTipStyle tip;
    tip.background = Brush::rounded(Color::hex(0x1A1A1AF2), rSmall, Color::hex(0x404040FF), 1.0f);
    tip.font = {FontStyle::Regular, 11.0f};
    tip.color = Color::hex(0xE0E0E0FF);
    tip.padding = Margin(6.0f, 3.0f);
    theme.set("ToolTip", tip);

    return theme;
}

}
