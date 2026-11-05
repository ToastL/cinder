---
type: concept
system: "[[Renderer]]"
source: "src/gfx/rhi/"
---

`gfx/rhi` is a neutral leaf that **declares** `Ctx`, `GpuBuffer`, `GraphicsPipeline`, `Texture`, `TexturePool`, `RenderTarget`, `GlyphPages` and `Presenter`. A backend **defines** them in namespace `cinder::gfx::rhi`. No vtables: `Engine` holds `rhi::Ctx ctx_` by value. See [[Decision - Compile-time backend]].

- `gfx/rhi/Backend.hpp` is **generated** by `configure_file` into the build tree: one line, `#include "gfx/<backend>/Backend.hpp"`. Everything above the backend includes only that.
- Nothing outside a backend directory names a Vulkan or Metal type.

**Three places where Metal differs, each shaping the seam:**
1. `beginScenePass`/`beginPresentPass` take `Frame&` — Metal makes a fresh encoder per pass.
2. `Commands::handle` points at a `CommandState` (encoder, current index buffer, push-constant staging), because Metal has no index-buffer binding point.
3. `TextureBinding::id` points at a texture/sampler pair, because Metal binds those separately.
