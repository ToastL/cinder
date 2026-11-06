---
type: class
system: "[[Editor]]"
source: "src/dev/PlaySession.hpp"
---

Owns the editor's state, `Edit | Playing | Paused`.

- **Play** saves the scene to text (`SceneCodec::save`) and hands it to `Engine::loadScene` — the exact path the player takes from disk. Every node is rebuilt, connections and timers start empty, and a serialization bug shows up on Play rather than Stop.
- **Stop** hands the same text back to `loadScene` and releases a locked cursor. Play and Stop are one operation; only whether the loop simulates afterwards differs.
- **Pause** → `GameLoop::idle`; **Step** → one `GameLoop::step`.
- Buttons and shortcuts only set a **request**, applied **between frames** in `PlaySession::tick`: the toolbar draws inside `Engine::render`, and Stop rebuilds every node, which must never happen while a node's code is on the stack.
- Game code calling `engine.quit()` becomes Stop (Unity ignores `Application.Quit` in the editor too).
- Save is disabled during Play. Entering Playing focuses the Scene view so the game gets the keyboard.
