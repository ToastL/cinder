---
type: module
system: "[[Editor]]"
source: "src/dev/Picking.hpp"
---

A CPU raycast that knows nothing about the UI. The click unprojects through the editor camera and hits:
- every enabled `MeshPart` as the unit cube (spheres and capsules too, for now),
- every enabled `Sprite` as its quad, in its own plane,
- every enabled `Camera` within 10 points of where it projects.

Nearest wins; a tie goes to the later node in render order (the sprite drawn on top). A disabled node hides its subtree.

A click is press + release within `Application::DRAG_THRESHOLD`; past it, a left drag looks around. A hit selects with `reveal` (the [[Explorer]] scrolls to the row); a miss clears. A press on a gizmo handle comes first and its release never picks.
