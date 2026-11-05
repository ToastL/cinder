---
type: system
status: live
layer: "text, ui/core, ui/framework, ui/widgets, ui/docking"
source: "src/text/, src/ui/"
started: 2026-09-22
last-update: 2026-09-25
---

## About

**What it is:** a retained UI of our own, shaped like Unreal's Slate ([[Decision - Own UI framework]]). Widgets persist between frames; each frame the `Application` asks for desired sizes bottom-up (`prepass`), arranges top-down (`arrangeChildren` hands out a `Geometry`), and paints into the renderer's `ui::ElementList`. The same paint builds the `HitTester`, so input routes against what was last drawn.

**Layers:** `text` (fonts, shaping, glyph atlas) → `ui/core` (widget, layout, events, elements, batcher) → `ui/framework` (`Application`, commands, platform hooks) → `ui/widgets` → `ui/docking`. Part of `engine`, because games will use it too; never includes `scene`, `reflect` or `gfx`.

**Test bench:** `ui_gallery` — every widget live, every primitive drawn.
```sh
DYLD_LIBRARY_PATH=/opt/homebrew/lib ./build/ui_gallery --frames 10 --capture-window gallery.png
```
`--lowdpi` turns off framebuffer scaling; `--text-gamma` tunes glyph coverage.

**Tests:** headless through [[UI harness]] — `ui_*_test`, `text_*_test`.

## Code
[[Text shaping]], [[Widgets and Args]], [[UI input routing]], [[Popups and tooltips]], [[TreeView]], [[Drag and drop]], [[Docking]], [[Theme]]

## Tasks
- [ ] Font fallback — a symbol font so shortcuts read ⌘⇧P instead of `Shift+Cmd+P`
- [ ] Floating panels in their own OS windows
- [ ] Game UI — reflected widget classes saved as `.widget` assets
- [ ] Project browser — [[Decision - Project browser deferred]]

## Notes
- 2026-09-22/23: Built in two days, then ImGui deleted.
- On macOS the editor is moving to SwiftUI ([[macOS editor]]); this UI stays the game UI and the Linux editor.
