---
type: system
status: live
layer: "reflect"
source: "src/reflect/"
started: 2026-09-09
last-update: 2026-09-21
---

## About

**What it is:** C++ has no runtime reflection, so props are declared with macros that stringify the field name. One declaration feeds the serializer and the Properties panel — a new `CINDER_PROP` needs zero editor, serializer or scripting code.

**Headers:** `Enum`, `PropDef`, `PropType`, `Reflect`.

**Connections:** used by [[Scene]] (every node class), [[Serialization]], [[Game code]] and [[Properties panel]]. `reflect::copyProps` copies through each prop's writer, including clamping and notifications.

## Code
[[Props]]

## Notes
- `PropValue` (the value type) lives in `scene`, not here. See [[Layer graph]].
