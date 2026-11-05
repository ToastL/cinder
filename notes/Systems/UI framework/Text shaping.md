---
type: module
system: "[[UI framework]]"
source: "src/text/"
---

`text`: `Font`, `FontSet`, `Shaper`, `TextLayout`, `GlyphAtlas`, `Utf8`. FreeType rasterizes, HarfBuzz shapes. Fonts are Roboto and Roboto Mono (OFL-1.1) in `engine/fonts/`.

The glyph atlas tracks dirty rectangles that [[UiRenderer]] uploads into R8 pages each frame. Roboto has no ⌘, so `Shortcut::label()` spells modifiers out until font fallback lands.
