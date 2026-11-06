---
type: concept
system: "[[Scene]]"
source: "Source/scene/Attributes.hpp"
---

Per-node free-form data: a `PropRec` on `Node`, saved as an `attributes { ... }` block, edited in [[Properties panel]], read by game code with `Node::attribute`.

- **Values:** number, string, bool, or a sequence of 2–4 numbers. **Names:** letters, digits, `_`.
- A change emits `Signal::AttributeChanged` only on a real change (`sameAttribute` counts `2` and `2.0` equal). `loadAttributes` never notifies.
- Typed settings belong in a game class's props; attributes are for data any node can carry.
- Properties edits every number as a float, because `TextLoad` reads `40` as an int and `40.5` as a float.

Limit: an attribute can't name a node (waits for node references).
