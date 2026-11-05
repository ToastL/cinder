---
type: class
system: "[[Core and game loop]]"
source: "src/core/Engine.hpp"
---

A library that gets ticked, not something that runs itself. **Don't move the loop into `Engine`.**

- The constructor boots Lua but **loads nothing and runs nothing**; the host decides what happens next.
- Owns `rhi::Ctx` (first), `Input` (before `Renderer`), the renderer, `Scene`, `NodeTypes`, the Lua host and a `physics::World`.
- `update(dt)`: `script->update`, `scene.update`, `physics.step`, `input.consume()`.
- `render(alpha)`: `renderer.beginFrame()`, `script->render(alpha)`, `scene.render(alpha, draws)`, `renderer.drawFrame()`.
- `loadScene(text)`: `scene.clear()`, then `LuaHost::boot()` (fresh `lua_State`), then `SceneCodec::load`. **Order is load-bearing:** every `Script` holds the `lua_State*` it was created with, so the scene must be cleared before `boot()` closes that state.
