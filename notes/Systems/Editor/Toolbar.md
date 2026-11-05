---
type: module
system: "[[Editor]]"
source: "src/dev/panels/Toolbar.hpp"
---

Play controls, save, undo and redo, the dirty marker, the ⌘ shortcuts as a `CommandList`, the Save / Don't Save / Cancel `Dialog`, and the File, Edit, Window, Play and Platforms menus.

- Its `update` calls `History::settle` ([[Undo history]]) and logs queued `[package]` lines from the [[Editor packager]] on the main thread.
- `requestClose` / `closeConfirmed` implement the unsaved-changes guard.
- The Window menu comes from `dev/panels/Layout` via [[Docking]]'s `fillWindowMenu`.
