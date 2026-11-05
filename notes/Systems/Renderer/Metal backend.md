---
type: module
system: "[[Renderer]]"
source: "src/gfx/mtl/"
---

Native Metal, macOS only (`-DCINDER_BACKEND=mtl`, the macOS default). Objective-C++ with `-fobjc-arc`; links Metal, QuartzCore, Foundation, CoreGraphics, AppKit.

- About half the Vulkan backend's size: no allocator, descriptor pools, layout barriers or explicit frame sync — those concepts are deleted, not ported.
- Headers are plain C++ holding `void*`, bridged inside the `.mm` files, because neutral `.cpp` files include them through the selector.
- `PipelineFactory` loads the build's `.metallib` and falls back to compiling the `.metal` source at runtime. See [[Shader pipeline]].
- Needs no `DYLD_LIBRARY_PATH` and no Vulkan SDK at runtime.
- The `Presenter` manages autorelease pools; a missing pool leaked ~55 GB of footprint on 2026-09-25. See [[Verification recipes]].
