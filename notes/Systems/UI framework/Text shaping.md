---
type: module
system: "[[UI framework]]"
source: "Source/text/"
---

`text`: `Font`, `FontSet`, `Shaper`, `TextLayout`, `GlyphAtlas`, `Utf8`, `TrueType`, `Rasterizer`. Our own code reads the font, kerns from GPOS and rasterizes by signed-area accumulation. No FreeType or HarfBuzz since 2026-11-26 ([[Decision - Own text stack]]). One glyph per codepoint: no ligatures, marks, RTL or hinting. Fonts are Roboto and Roboto Mono (OFL-1.1) in `Content/Fonts/`.

The glyph atlas tracks dirty rectangles that [[UiRenderer]] uploads into R8 pages each frame. Roboto has no ⌘, so `Shortcut::label()` spells modifiers out until font fallback lands.
