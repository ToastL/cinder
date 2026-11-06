---
type: system
status: live
layer: "gfx, gfx/rhi, gfx/vk, gfx/mtl, gfx/asset, gfx/pass"
source: "src/gfx/"
started: 2026-09-09
last-update: 2026-09-25
---

## About

**What it is:** the renderer, split five ways around a backend seam. `gfx/rhi` declares the types a backend provides; `gfx/vk` (Vulkan) or `gfx/mtl` (Metal, macOS only) defines them, picked at compile time. Both backends are pixel-identical on the gallery and both sandboxes.

**The five parts:**
- `gfx/rhi` — the seam: formats, `Commands`/`Uploads`, `TextureBinding`, `PipelineBuilder`, `Capture`. See [[Backend seam]].
- `gfx/vk`, `gfx/mtl` — [[Vulkan backend]], [[Metal backend]].
- `gfx/asset` — what you draw with: `Assets`, `Mesh`.
- `gfx/pass` — how you draw it: [[DrawPass]] and its implementations, their pipelines, [[ViewCamera]].
- `gfx` — orchestration: `Renderer`, `FrameTargets`, `UiRenderer`, `CompositePipeline`, `RendererDrawList`.

**Connections:** the scene reaches it through `DrawList` ([[Scene]]); the editor through `setUiPaint`, `setViewportSize` and `overrideCamera` ([[Scene view]]); game code through `game/Game.hpp` ([[Game code]]).

**Run with validation:** `DYLD_LIBRARY_PATH=/opt/homebrew/lib` — see [[Vulkan on Apple Silicon]].

## Code
[[Backend seam]], [[Vulkan backend]], [[Metal backend]], [[Frame structure]], [[DrawPass]], [[ViewCamera]], [[UiRenderer]], [[Texture and mesh names]]

## Tasks
- [ ] Lights and a material model (one hardcoded directional light today), shadow maps
- [ ] Sorting (none in either pass), frustum culling, instancing (one draw per mesh)
- [ ] Mipmaps (`mipLevels = 1`), anisotropy, MSAA, post chain
- [ ] Device-local memory — every `GpuBuffer` is host-visible
- [ ] Fixed caps: `MAX_QUADS = 10000`, `MAX_DRAWS = 4096`, `MAX_TEXTURES = 256`
- [ ] Resizing the Scene panel stalls the device (`vkDeviceWaitIdle` per size change)
- [ ] Swapchain format changes leave scene/composite/overlay pipelines in place
- [ ] `endSingleTime` does a full `vkQueueWaitIdle` per texture upload
- [ ] Render interpolation (`alpha` unused)

## Notes
- 2026-09-23: The seam and the Metal backend landed (PR #5).
- 2026-09-25: Metal autorelease-pool leak fixed in `Presenter`. See [[Verification recipes]].
