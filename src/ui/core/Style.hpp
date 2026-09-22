#pragma once

#include "text/FontSet.hpp"
#include "ui/core/Layout.hpp"
#include "ui/core/Color.hpp"
#include "ui/core/Rect.hpp"

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include <any>
#include <functional>
#include <map>
#include <string>
#include <string_view>

namespace cinder::ui {

class ElementList;

struct PaintStyle {
    Color tint = Color::white();
    Color foreground = Color::white();

    Color apply(Color color) const { return color * tint; }
};

struct Brush {
    enum class DrawAs : std::uint8_t { None, Box, Image };

    DrawAs drawAs = DrawAs::None;
    Color fill = Color::transparent();
    Color outline = Color::transparent();
    float outlineWidth = 0.0f;
    glm::vec4 radii{0.0f};
    std::string image;
    glm::vec2 imageSize{0.0f};

    static Brush none() { return {}; }
    static Brush color(Color fill, float radius = 0.0f) {
        return {DrawAs::Box, fill, Color::transparent(), 0.0f, glm::vec4(radius), {}, {}};
    }
    static Brush rounded(Color fill, float radius, Color outline = Color::transparent(),
                         float outlineWidth = 0.0f) {
        return {DrawAs::Box, fill, outline, outlineWidth, glm::vec4(radius), {}, {}};
    }
    static Brush picture(std::string path, glm::vec2 size, Color tint = Color::white()) {
        return {DrawAs::Image, tint, Color::transparent(), 0.0f, glm::vec4(0.0f), std::move(path), size};
    }

    void paint(ElementList& list, int layer, const Rect& rect, const PaintStyle& style, float scale = 1.0f) const;
};

struct FontInfo {
    cinder::text::FontStyle face = cinder::text::FontStyle::Regular;
    float size = 13.0f;

    bool operator==(const FontInfo&) const = default;
};

struct LabelStyle {
    FontInfo font;
    Color color = Color::white();
};

struct ButtonStyle {
    Brush normal;
    Brush hovered;
    Brush pressed;
    Brush disabled;
    Margin padding{10.0f, 4.0f};
    Color foreground = Color::white();
    Color disabledForeground = Color::white();
};

struct CheckBoxStyle {
    Brush box;
    Brush hovered;
    Brush checked;
    Brush checkedHovered;
    Color mark = Color::white();
    float size = 14.0f;
    float spacing = 6.0f;
};

struct TextFieldStyle {
    FontInfo font;
    Color color = Color::white();
    Color hint = Color::white();
    Color selection = Color::white();
    Color caret = Color::white();
};

struct TextBoxStyle {
    Brush normal;
    Brush hovered;
    Brush focused;
    Brush disabled;
    Margin padding{6.0f, 3.0f};
    TextFieldStyle text;
};

struct ScrollBarStyle {
    Brush track;
    Brush thumb;
    Brush thumbHovered;
    Brush thumbDragged;
    float thickness = 8.0f;
    float minThumb = 16.0f;
};

struct SplitterStyle {
    Brush handle;
    Brush handleHovered;
    float handleSize = 4.0f;
};

struct SpinBoxStyle {
    Brush normal;
    Brush hovered;
    Brush active;
    Brush editing;
    Brush fill;
    Brush fillHovered;
    Margin padding{6.0f, 3.0f};
    float accentWidth = 4.0f;
};

struct ExpandableAreaStyle {
    Brush header;
    Brush headerHovered;
    Brush body;
};

struct MenuStyle {
    Brush background;
    Brush highlight;
    Margin padding{4.0f};
    Margin entryPadding{8.0f, 4.0f};
    FontInfo font;
    Color color = Color::white();
    Color dim = Color::white();
    Color separator = Color::white();
    float checkWidth = 20.0f;
    float shortcutGap = 24.0f;
    float minWidth = 140.0f;
};

struct MenuBarStyle {
    Brush background;
    Brush item;
    Brush itemHovered;
    Brush itemOpen;
    Margin itemPadding{8.0f, 3.0f};
    FontInfo font;
    Color color = Color::white();
};

struct ColorPickerStyle {
    Brush background;
    Brush swatchBorder;
    Color checkerLight = Color::white();
    Color checkerDark = Color::black();
    Color marker = Color::white();
    float checkerSize = 6.0f;
};

struct ToolTipStyle {
    Brush background;
    FontInfo font;
    Color color = Color::white();
    Margin padding{8.0f, 4.0f};
};

class Theme {
public:
    template <typename T>
    void set(std::string name, T value) {
        entries_.insert_or_assign(std::move(name), std::any(std::move(value)));
    }

    template <typename T>
    const T& get(std::string_view name) const {
        const auto found = entries_.find(name);
        if (found != entries_.end()) {
            if (const T* value = std::any_cast<T>(&found->second)) return *value;
        }
        missing(name);
        static const T fallback{};
        return fallback;
    }

    template <typename T>
    bool has(std::string_view name) const {
        const auto found = entries_.find(name);
        return found != entries_.end() && std::any_cast<T>(&found->second) != nullptr;
    }

    Color color(std::string_view name) const { return get<Color>(name); }

private:
    void missing(std::string_view name) const;

    std::map<std::string, std::any, std::less<>> entries_;
};

}
