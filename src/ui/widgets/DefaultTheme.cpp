#include "ui/widgets/DefaultTheme.hpp"

namespace cinder::ui {

using cinder::text::FontStyle;

Theme defaultTheme() {
    Theme theme;

    const Color black = Color::hex(0x000000FF);
    const Color title = Color::hex(0x0F0F0FFF);
    const Color background = Color::hex(0x151515FF);
    const Color recessed = Color::hex(0x1A1A1AFF);
    const Color panel = Color::hex(0x242424FF);
    const Color header = Color::hex(0x2F2F2FFF);
    const Color input = Color::hex(0x0F0F0FFF);
    const Color dropdown = Color::hex(0x383838FF);
    const Color outline = Color::hex(0x4C4C4CFF);
    const Color hover = Color::hex(0x575757FF);
    const Color hoverBright = Color::hex(0x808080FF);
    const Color foreground = Color::hex(0xC0C0C0FF);
    const Color bright = Color::hex(0xFFFFFFFF);
    const Color dim = Color::hex(0x808080FF);
    const Color primary = Color::hex(0x0070E0FF);
    const Color primaryHover = Color::hex(0x0E86FFFF);
    const Color primaryPress = Color::hex(0x0050A0FF);

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
    theme.set("Color.Error", Color::hex(0xEF3535FF));
    theme.set("Color.Warning", Color::hex(0xFFB800FF));
    theme.set("Color.Success", Color::hex(0x8BC24AFF));
    theme.set("Color.AxisX", Color::hex(0xE84848FF));
    theme.set("Color.AxisY", Color::hex(0x78CC50FF));
    theme.set("Color.AxisZ", Color::hex(0x4880F0FF));
    theme.set("Color.AxisW", Color::hex(0xB0B0B0FF));

    theme.set("Brush.Background", Brush::color(background));
    theme.set("Brush.Recessed", Brush::color(recessed));
    theme.set("Brush.Panel", Brush::color(panel));
    theme.set("Brush.Header", Brush::color(header));
    theme.set("Brush.Title", Brush::color(title));

    theme.set("Label", LabelStyle{{FontStyle::Regular, 13.0f}, foreground});
    theme.set("Label.Small", LabelStyle{{FontStyle::Regular, 11.0f}, foreground});
    theme.set("Label.Bold", LabelStyle{{FontStyle::Bold, 13.0f}, foreground});
    theme.set("Label.Header", LabelStyle{{FontStyle::Bold, 14.0f}, bright});
    theme.set("Label.Mono", LabelStyle{{FontStyle::Mono, 12.0f}, foreground});

    ButtonStyle button;
    button.normal = Brush::rounded(dropdown, 4.0f);
    button.hovered = Brush::rounded(hover, 4.0f);
    button.pressed = Brush::rounded(header, 4.0f);
    button.disabled = Brush::rounded(recessed, 4.0f);
    button.padding = Margin(10.0f, 4.0f);
    button.foreground = foreground;
    button.disabledForeground = Color::hex(0x606060FF);
    theme.set("Button", button);

    ButtonStyle primaryButton = button;
    primaryButton.normal = Brush::rounded(primary, 4.0f);
    primaryButton.hovered = Brush::rounded(primaryHover, 4.0f);
    primaryButton.pressed = Brush::rounded(primaryPress, 4.0f);
    primaryButton.foreground = bright;
    theme.set("Button.Primary", primaryButton);

    ButtonStyle toolbar = button;
    toolbar.normal = Brush::none();
    toolbar.hovered = Brush::rounded(dropdown, 4.0f);
    toolbar.pressed = Brush::rounded(header, 4.0f);
    toolbar.disabled = Brush::none();
    toolbar.padding = Margin(6.0f, 4.0f);
    theme.set("Button.Toolbar", toolbar);

    CheckBoxStyle check;
    check.box = Brush::rounded(input, 2.0f, hover, 1.0f);
    check.hovered = Brush::rounded(input, 2.0f, hoverBright, 1.0f);
    check.checked = Brush::rounded(primary, 2.0f);
    check.checkedHovered = Brush::rounded(primaryHover, 2.0f);
    check.mark = bright;
    theme.set("CheckBox", check);

    TextFieldStyle field;
    field.font = {FontStyle::Regular, 13.0f};
    field.color = Color::hex(0xE0E0E0FF);
    field.hint = Color::hex(0x6A6A6AFF);
    field.selection = primary.withAlpha(0.45f);
    field.caret = bright;
    theme.set("TextField", field);

    TextFieldStyle mono = field;
    mono.font = {FontStyle::Mono, 12.0f};
    theme.set("TextField.Mono", mono);

    TextBoxStyle box;
    box.normal = Brush::rounded(input, 4.0f, dropdown, 1.0f);
    box.hovered = Brush::rounded(input, 4.0f, hover, 1.0f);
    box.focused = Brush::rounded(input, 4.0f, primary, 1.0f);
    box.disabled = Brush::rounded(background, 4.0f, recessed, 1.0f);
    box.padding = Margin(6.0f, 3.0f);
    theme.set("TextBox", box);

    ScrollBarStyle scroll;
    scroll.track = Brush::none();
    scroll.thumb = Brush::rounded(hover, 3.0f);
    scroll.thumbHovered = Brush::rounded(hoverBright, 3.0f);
    scroll.thumbDragged = Brush::rounded(Color::hex(0xA0A0A0FF), 3.0f);
    scroll.thickness = 10.0f;
    theme.set("ScrollBar", scroll);

    SplitterStyle splitter;
    splitter.handle = Brush::color(title);
    splitter.handleHovered = Brush::color(primary);
    splitter.handleSize = 4.0f;
    theme.set("Splitter", splitter);

    SpinBoxStyle spin;
    spin.normal = Brush::rounded(input, 4.0f, dropdown, 1.0f);
    spin.hovered = Brush::rounded(input, 4.0f, hover, 1.0f);
    spin.active = Brush::rounded(input, 4.0f, primary, 1.0f);
    spin.editing = Brush::rounded(input, 4.0f, primary, 1.0f);
    spin.fill = Brush::rounded(dropdown, 4.0f);
    spin.fillHovered = Brush::rounded(hover, 4.0f);
    spin.padding = Margin(6.0f, 3.0f);
    spin.accentWidth = 4.0f;
    theme.set("SpinBox", spin);

    ButtonStyle combo = button;
    combo.normal = Brush::rounded(input, 4.0f, dropdown, 1.0f);
    combo.hovered = Brush::rounded(input, 4.0f, hover, 1.0f);
    combo.pressed = Brush::rounded(recessed, 4.0f, hover, 1.0f);
    combo.disabled = Brush::rounded(background, 4.0f, recessed, 1.0f);
    combo.padding = Margin(6.0f, 3.0f);
    theme.set("ComboButton", combo);

    ExpandableAreaStyle area;
    area.header = Brush::color(header);
    area.headerHovered = Brush::color(Color::hex(0x363636FF));
    area.body = Brush::none();
    theme.set("ExpandableArea", area);

    MenuStyle menu;
    menu.background = Brush::rounded(Color::hex(0x1F1F1FFF), 4.0f, outline, 1.0f);
    menu.highlight = Brush::rounded(primary, 3.0f);
    menu.padding = Margin(4.0f);
    menu.entryPadding = Margin(8.0f, 4.0f);
    menu.font = {FontStyle::Regular, 13.0f};
    menu.color = Color::hex(0xDADADAFF);
    menu.dim = dim;
    menu.separator = outline;
    theme.set("Menu", menu);
    theme.set("Menu.Heading", LabelStyle{{FontStyle::Bold, 11.0f}, dim});

    MenuBarStyle bar;
    bar.background = Brush::none();
    bar.item = Brush::none();
    bar.itemHovered = Brush::rounded(dropdown, 3.0f);
    bar.itemOpen = Brush::rounded(primary, 3.0f);
    bar.itemPadding = Margin(8.0f, 3.0f);
    bar.font = {FontStyle::Regular, 13.0f};
    bar.color = foreground;
    theme.set("MenuBar", bar);

    ColorPickerStyle picker;
    picker.background = Brush::rounded(panel, 6.0f, outline, 1.0f);
    picker.swatchBorder = Brush::rounded(Color::transparent(), 2.0f, black, 1.0f);
    picker.checkerLight = Color::hex(0xB4B4B4FF);
    picker.checkerDark = Color::hex(0x6E6E6EFF);
    picker.marker = bright;
    picker.checkerSize = 6.0f;
    theme.set("ColorPicker", picker);

    TableViewStyle table;
    table.background = Brush::none();
    table.rowHovered = Brush::color(Color::hex(0x2E2E2EFF));
    table.rowSelected = Brush::color(primary);
    table.rowSelectedInactive = Brush::color(Color::hex(0x3A4450FF));
    table.dropTarget = primaryHover;
    table.rowPadding = Margin(4.0f, 0.0f);
    table.indent = 14.0f;
    table.rowHeight = 22.0f;
    theme.set("TableView", table);

    ToolTipStyle tip;
    tip.background = Brush::rounded(Color::hex(0x0A0A0AF2), 3.0f, outline, 1.0f);
    tip.font = {FontStyle::Regular, 12.0f};
    tip.color = Color::hex(0xDADADAFF);
    tip.padding = Margin(8.0f, 5.0f);
    theme.set("ToolTip", tip);

    return theme;
}

}
