---
type: module
system: "[[Editor]]"
source: "Source/dev/panels/Explorer.hpp"
---

A [[TreeView]] over node **ids**: builds only rows in view and nothing can dangle — each row reads its node from the scene every frame.

- A row is the name, then the class dimmed. Disabled nodes are dimmed unless selected.
- Selection is bound to [[Selection]] through `isItemSelected` — picking in the Scene view and clicking a row are the same state.
- Click selects, click on empty space clears. **"+"** lists every registered class and inserts under the selection or at the root.
- Right-click: Insert, Duplicate (`Scene::clone`), Delete. Drag a row onto another to reparent (cycles refused); drop below the rows to make it a root.
- Delete/Backspace removes the selection while the tree has focus. Delete uses `destroyNow` (Edit mode never runs `update`).
- A filter turns it into a flat list of matches by name or class.
- During Play, edits act on the running game and Stop discards them.
