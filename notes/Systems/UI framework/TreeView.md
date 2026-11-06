---
type: module
system: "[[UI framework]]"
source: "Source/ui/widgets/TreeView.hpp"
---

**Builds only the rows that fit.** Items are ids, never pointers, so Play, Stop and Undo rebuilding every node can't invalidate a row. Each frame it flattens expanded items through `treeItemsSource` and `onGetChildren`, realizes the visible slice (reusing rows), drops the rest — a thousand items cost a dozen widgets. A `ListView` is a `TreeView` with no children callback.

- Selection is the tree's own, or the panel's when `isItemSelected` is bound ([[Explorer]] binds it to [[Selection]]).
- Expander press only expands; row press selects and arms a drag; double click expands or calls `onMouseButtonDoubleClick`; right press selects and opens `onContextMenuOpening`.
- Arrow keys move, expand and collapse; `reveal` expands ancestors and scrolls into view.
- Drop zones: above, onto or below a row, or the empty space below — painted as an outline or line.
