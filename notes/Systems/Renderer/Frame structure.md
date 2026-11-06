---
type: concept
system: "[[Renderer]]"
source: "Source/gfx/Renderer.cpp, Source/gfx/FrameTargets.hpp"
---

Two passes, two frames in flight:

1. **Scene pass** (colour + depth) runs the ordered [[DrawPass]] list — `MeshPass`, then `SpritePass` — into a `RenderTarget`, ending in shader-read layout.
2. **Present pass** draws one fullscreen triangle (`CompositePipeline`) sampling that target into the swapchain, then the UI ([[UiRenderer]]).

- **One target per frame in flight** (`FrameTargets`): a single one would be cleared by frame N+1 while frame N's composite still sampled it.
- The target uses the swapchain's sRGB format, so encode/decode is identity; UNORM would brighten everything.
- The scene pass has a second subpass dependency (`0 → EXTERNAL`, colour-write → fragment-read) ordering the composite after the scene. Any new pass reading a previous pass's output needs the same.
- **Embedding:** `setViewportSize(w, h)` in window points detaches the target from the window: both targets rebuild at `w×h × scale`, cameras resize, and the composite stops — the [[Scene view]] draws the target as an image. Rebuilds sit behind `vkDeviceWaitIdle`. `player` never calls it.
- **Captures:** `capture(path)` reads back the scene target (`gfx/Capture`); `requestWindowCapture(path)` copies the next swapchain image, adding `TRANSFER_SRC` the first time. Both stall — debug tools only.
