---
type: module
system: "[[UI framework]]"
source: "Source/text/"
---

`text`: `Font`, `FontSet`, `Shaper`, `TextLayout`, `GlyphAtlas`, `Utf8`. FreeType rasterizes, HarfBuzz shapes. Fonts are Roboto and Roboto Mono (OFL-1.1) in `Content/Fonts/`.

The glyph atlas tracks dirty rectangles that [[UiRenderer]] uploads into R8 pages each frame. Roboto has no ⌘, so `Shortcut::label()` spells modifiers out until font fallback lands.
