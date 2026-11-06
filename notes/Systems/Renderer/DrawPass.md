---
type: concept
system: "[[Renderer]]"
source: "Source/gfx/pass/DrawPass.hpp"
---

The extension point: `beginFrame` / `record`. Named `DrawPass` so that `renderPass` unambiguously means a Vulkan render pass.

**A new pass:** implement the interface and add it to `passes_` in the `Renderer` constructor in draw order. `Renderer::sprites()`/`meshes()` hand the built-in passes to `game/Game` for immediate-mode drawing. A pass owns no camera — `record` gets the frame's view-projection.

**`MeshPass`** — primitives `cube`, `sphere`, `capsule`; one hardcoded directional light; one draw per object.

**`SpritePass`** — a batch with atlas support (`SpriteBatch`, run-based: only consecutive quads sharing a texture merge). Tests depth without writing it (`depthRead`), so meshes hide sprites while sprites layer in submission order. A `Sprite` is a quad of `size` in its local XY plane through its world matrix; the texture's first row runs along +Y. Game code: `game::drawSprite`, `drawSpriteRegion`, `drawRect` place a rectangle on `z = 0` with `x, y` at its bottom-left.

The UI is never a `DrawPass`: every pass records in the scene pass, which has depth. See [[UiRenderer]].
