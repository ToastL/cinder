---
type: concept
system: "[[Renderer]]"
source: "Source/gfx/RendererDrawList.cpp, Source/gfx/asset/Assets.hpp"
---

**Nodes never store GPU handles** — a handle means nothing on the next run. `Sprite.texture`, `MeshPart.texture` and `MeshPart.mesh` are *names*: a path under `Content/` or a primitive like `"cube"`.

- Resolved through `DrawList::textureHandle` / `meshHandle` on first draw, cached until `propChanged` says the name changed ([[Props]]).
- `RendererDrawList` caches by name, so a thousand sprites on one texture resolve its path once.
- A missing file logs once and draws white; an unknown mesh logs once and draws a cube.
- `Assets` hands out `int` handles (`DrawList::WHITE == 0`), caches by path, never frees. `MAX_TEXTURES = 256`, a fixed descriptor pool.
- Clear colour and 2D virtual size are `Camera` props, pushed every frame through `DrawList::background` and `camera`.
