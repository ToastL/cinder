---
type: class
system: "[[Editor]]"
source: "Source/dev/History.hpp"
---

`dev/History` stores **whole scenes**, not commands: after every finished edit it saves the scene to text; undo loads the previous text with `SceneCodec::load`. Nothing has to describe an edit to make it undoable — a new `CINDER_PROP` is undoable with zero editor code.

- Panels and gizmos call `touch(label, selection)` when they write. `Toolbar::update` calls `settle(app.isInteracting())`, which commits only once nothing holds the mouse capture, no text is being edited and no drag-and-drop is in flight. A forty-frame drag is one step; an edit that saves to the same text records nothing. `MAX_STEPS` is 100.
- Undo calls the codec directly, not `loadScene`. History is Edit-mode-only: during Play, `touch`, `settle` and the buttons do nothing.
- **Dirty** while the last commit's text differs from the text last saved/opened, so undoing back to the save is clean. The toolbar shows `main.scene*`.
- **Close:** `editor/main.cpp` clears GLFW's close flag and calls `Toolbar::requestClose` — confirms at once when clean, else Save / Don't Save / Cancel. The loop runs until `closeConfirmed()`.
