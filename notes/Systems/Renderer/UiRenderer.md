---
type: module
system: "[[Renderer]]"
source: "src/gfx/UiRenderer.hpp"
---

Draws a `ui::ElementList` in the present pass, after the composite.

- `Renderer::setUiPaint` installs a hook called by `beginFrame` with a fresh list in window points. Paint runs before the frame fence, so it fills CPU arrays only.
- `prepare`, recorded before the scene pass, batches with the GPU-free `ui::batch`, fills that frame's growable host-visible buffers, and uploads dirty glyph-atlas rectangles into R8 pages (barriers from shader-read so existing glyphs survive).
- **One pipeline, a mode per vertex:** solid, textured, glyph coverage with gamma, rounded-box SDF for fills and borders, and an opaque image for the scene target. Only texture-sampling elements break a batch.
- Output is premultiplied; a non-sRGB swapchain gets a manual encode.
- `pixelsPerPoint` is measured only when the swapchain rebuilds, so a live resize doesn't rasterize glyphs at a stream of fractional sizes.

See [[UI framework]].
