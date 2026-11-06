---
type: module
system: "[[UI framework]]"
source: "Source/ui/docking/TabManager.hpp"
---

**A layout tree, not widget surgery.** `TabManager` holds a `LayoutNode` tree of splits and stacks; every change (drop, close, reopen) edits the tree and rebuilds widgets from it, carrying splitter sizes over first — nothing reparents a live `Splitter`.

- A tab's panel is spawned once and cached by id, so re-docking the Scene view keeps its camera and render target.
- Dragging a tab carries a `TabDragDrop`; the stack under the pointer answers with a side — middle joins, an edge splits — painted as a translucent overlay.
- Closing a stack's last tab collapses it; a split left with one child is replaced by that child.
- `fillWindowMenu` builds the Window menu: a check per tab, disabled for tabs that can't close (Scene).
- A tab bar keeps its `DockTab`s across an activation, because rebuilding mid-press would destroy the widget whose drag was just armed.
- Layout isn't persisted between runs yet.
