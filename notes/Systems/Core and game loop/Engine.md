---
type: class
system: "[[Core and game loop]]"
source: "Source/core/Engine.hpp"
---

A library that gets ticked, not something that runs itself. **Don't move the loop into `Engine`.**

- The constructor **loads nothing and runs nothing**; the host decides what happens next. It sets `Engine::current()`, which `game/Game.hpp` uses.
- Owns `rhi::Ctx` (first), `Input` (before `Renderer`), the renderer, `NodeTypes`, `Scene` and a `physics::World`.
- `update(dt)`: `scene.update` (starts, timers, the tree), `physics.step`, `input.consume()`.
- `render(alpha)`: `renderer.beginFrame()`, `scene.render(alpha, draws)`, `renderer.drawFrame()`.
- `loadScene(text)`: `scene.clear()`, then `SceneCodec::load`. Play and Stop both go through it.
- `quit()` only sets a flag: the player ends; the editor turns it into Stop.
