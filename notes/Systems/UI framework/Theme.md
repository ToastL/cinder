---
type: concept
system: "[[UI framework]]"
source: "src/ui/core/Style.hpp, src/ui/widgets/DefaultTheme.cpp"
---

Styles come from a `Theme` of named entries — `"Button"`, `"Button.Primary"`, `"Label.Mono"`, `"Color.Primary"` — built by `ui::defaultTheme()` in the colours of Unreal's dark editor. Colours are linear `ui::Color`s authored as sRGB hex; the swapchain encodes them. Editor-specific styles are in `dev/panels/EditorStyle`.
