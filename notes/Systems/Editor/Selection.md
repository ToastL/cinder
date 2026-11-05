---
type: class
system: "[[Editor]]"
source: "src/dev/Selection.hpp"
---

Holds the selected node's **id**, never a `Node*`. Play and Stop rebuild every node through `loadScene`, and ids round-trip through the codec, so a selection survives both.

`resolve` returns null — and forgets the id — once the node is gone or marked destroyed, so a node spawned during Play drops out of the selection on Stop. [[Explorer]] binds its tree's selection straight to it.
