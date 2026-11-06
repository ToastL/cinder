---
type: module
system: "[[Editor]]"
source: "Source/dev/panels/Properties.hpp"
---

Rows come from the prop list, with **no per-class widget code**: name, class and id; a Transform section for spatial nodes; the class's props; attributes. Each section is an `ExpandableArea`.

- Widget by `PropType` and `PropHint`: `SpinBox` (numbers), `VectorInputBox`, `CheckBox`, `TextBox`, `ComboBox` from `PropDef::options()` (enums), `ColorBlock` + picker (`CINDER_PROP_COLOR`).
- Every write goes through `PropDef`, so clamping, in-place vector writes and `propChanged` behave as from the serializer. Bounds only when declared; never rounds to the three decimals it shows.
- `CINDER_PROP_ANGLE` is dragged in degrees and written in radians.
- Text commits on Enter or focus loss, so a texture path doesn't load every prefix.
- Holds only the selected **id**; rebuilds rows only when the selection or attribute names change.
- **Attributes:** remove buttons and **Add Attribute…** (name + type popup). Numbers are edited as floats. Writes go through `Node::setAttribute`, so Play edits fire the game's signals.
