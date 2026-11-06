---
type: concept
system: "[[UI framework]]"
source: "Source/ui/framework/Application.hpp"
---

Input comes from `platform::Input`'s event queue ([[Input]]) via `Application::processEvents`. Routing is Slate's:

- **Mouse** bubbles from the widget under the pointer up its path until a `Reply` says handled, or goes straight to the captor. A press focuses the deepest focusable widget, or clears focus — how a text field commits on click-away.
- **Keys** bubble from the focused widget; only unhandled keys reach the global `CommandList`s, so a text field keeps ⌘Z.
- Hover is recomputed after every paint. Double click within `DOUBLE_CLICK_TIME`/`DISTANCE`. `Reply::detectDrag` fires `onDragDetected` past `DRAG_THRESHOLD`.
- Disabled widgets are still hit (for tooltips), but none of their handlers run.
- `Application::navigate` is Tab: focusable widgets in paint order, Shift backwards, staying inside the focused popup.
- `setFocus`, `pushPopup` and dismissals set the thread's current `Application`, so panels may call them from `update`.
