---
type: concept
system: "[[UI framework]]"
source: "src/ui/core/Args.hpp, src/ui/core/Widget.hpp"
---

Widgets are built with `ui::make<T>()`, which returns the widget's `Args`; chained setters fill them, `[child]` sets content, `+ T::slot()` adds a slot, and the `Args` convert to `std::shared_ptr<T>` by building. `.assign(ptr)` keeps a handle, like `SAssignNew`.

```cpp
auto toolbar = ui::make<HorizontalBox>()
    + HorizontalBox::slot().autoWidth().padding(4)
    [
        ui::make<Button>().text("Play").onClicked([&] { session.requestPlay(); return Reply::handled(); })
    ];
```

- A widget declares `struct Args : ui::Args<Args, Widget>` with `UI_ATTR`, `UI_ARG`, `UI_EVENT`, `UI_CONTENT`, `UI_SLOTS` and implements `construct(const Args&)`. Every widget gets visibility, enabled, tooltip and cursor.
- `Attribute<T>` holds a value or a getter called when read — how a label follows `history.title()`.
- `auto x = ui::make<T>()...` is the `Args`, not the widget: name the type or call `.build()`.
- Slot types live at namespace scope (clang can't use a nested class's member initializers inside the enclosing class).

**Widgets:** `Border`, `BoxPanel`, `Button`, `Canvas`, `ColorPicker`, `ComboBox`, `ExpandableArea`, `Label`, `Menu`, `ScrollBox`, `SizeBox`, `SpinBox`, `Splitter`, `TextField`, `TreeView`, `VectorInputBox`, `Viewport`.

- `SpinBox` keeps values exact: drag adds `step` per point (Shift ×10, Alt ×0.1), typing evaluates arithmetic, unchanged edits write nothing, clamps only to given bounds.
- `ColorPicker` works in sRGB-encoded HSV over a linear value, keeping hue through greys.
