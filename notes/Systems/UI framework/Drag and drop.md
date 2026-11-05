---
type: concept
system: "[[UI framework]]"
source: "src/ui/core/DragDrop.hpp"
---

The `Application`'s, like popups. `Reply::beginDragDrop(operation)` from `onDragDetected` starts it. While it runs, moves route `onDragEnter`/`onDragOver`/`onDragLeave`, the release routes `onDrop`, and the operation's `decorator()` is painted at the cursor (clamped, never hit). Escape or losing the window cancels; `onDropped(accepted)` reports either way.

Used by [[TreeView]] (reparenting in the [[Explorer]]) and [[Docking]] (`TabDragDrop`).
