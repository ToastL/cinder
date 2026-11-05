---
type: concept
system: "[[UI framework]]"
source: "src/ui/framework/Application.hpp, src/ui/widgets/Menu.hpp"
---

**Popups belong to the `Application`, not a widget.** `pushPopup(content, anchor, options)` places content below, beside or at a point next to an anchor, flipped and clamped into the window, painted above the whole tree.

- A press outside every popup closes them all and goes nowhere else (it can't pick in the Scene view on the way). A press in a lower popup closes the ones above. A press on a popup's `owner` routes normally so the owner can toggle.
- An unhandled Escape closes the top popup; focus returns to what held it before.
- Every menu, combo list, submenu, context menu and colour picker is a popup.
- **Menus:** `MenuBuilder` — entries, checks, headings, separators, submenus (hover after `SUBMENU_DELAY` or arrow keys), and `command(list, command)`, which takes label, shortcut, enabled and check from the `CommandList`, so a menu entry and a shortcut can't disagree. `MenuBar` opens on press and follows the pointer: press, drag, release picks.
- **Tooltips:** after `TOOLTIP_DELAY` over the deepest widget with `toolTipText` (disabled included), drawn beside the cursor, unhittable. A press hides it until the pointer leaves.
- Anything reading the hit grid during paint finds it empty; a menu row anchors its submenu to the rect it last painted.
