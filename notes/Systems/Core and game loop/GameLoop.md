---
type: class
system: "[[Core and game loop]]"
source: "Source/core/GameLoop.hpp"
---

The shared loop. Three entry points:

- **`tick`** — `pollEvents`, `engine.beginFrame()` (script hot reload); minimized → `waitEvents`, reset clock, skip; `advance()` accumulates real time clamped at `MAX_FRAME_TIME = 0.25s`; `engine.update(fixedDt)` N times; `engine.render(alpha())` once.
- **`idle`** — Edit mode and Paused: poll, reset the clock, `input.consume()`, render. Consuming here is load-bearing: otherwise the key that pressed Play (or an Esc from Edit mode) fires on the first play step.
- **`step`** — exactly one fixed update, then render (the Step button).

`advance()` and `alpha()` are public so tests drive the loop without a window (`loop_test`). Keep new loop logic in that shape.

`fixedHz` comes from [[Game.ini]].
