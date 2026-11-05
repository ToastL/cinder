---
type: module
system: "[[Platform]]"
source: "src/platform/Input.hpp"
---

Two interfaces over one input stream:

- **Edge flags for the game:** `keyPressed`/`keyReleased` are true for exactly one fixed update, cleared by `consume()` at the end of `Engine::update` and on every idle frame. Consuming on idle frames is load-bearing: otherwise the key that pressed Play fires on the first step. See [[GameLoop]].
- **Event queue for the UI:** `setRecording(true)` switches it on; the host drains it with `takeEvents()`. Keys carry repeats and modifiers (⌘ is `modifiers::PRIMARY` on macOS), characters arrive separately, cursor moves coalesce, focus loss synthesizes releases. `consume()` never touches the queue.

**Editor hooks:** `setViewportOrigin` makes `mouseX`/`mouseY` relative to the Scene panel; `setSuppressed` hides keyboard or mouse from the game — see [[Scene view]].

`Input` installs its GLFW callbacks in its constructor; `Engine` declares `input_` before `renderer_`.
