---
type: concept
system: "[[Scene]]"
source: "src/scene/Attributes.hpp"
---

Per-node settings are **attributes**: a `PropRec` on `Node`, saved as an `attributes { ... }` block, edited in [[Properties panel]], read by scripts.

- **Values:** number, string, bool, or a sequence of 2–4 numbers. **Names:** letters, digits, `_`.
- An attribute notifies only on a real change (`sameAttribute` counts `2` and `2.0` equal). `loadAttributes` never notifies.
- **Lua (Roblox's API, camelCase):** `getAttribute` (nil when missing, vectors for 2–4 numbers), `setAttribute` (nil removes), `getAttributes`, `getAttributeChangedSignal(name)`, `node.attributeChanged`. Invalid name or value raises a Lua error.
- Attributes survive [[Hot reload]] because they live on the node.
- Properties edits every number as a float, because `TextLoad` reads `40` as an int and `40.5` as a float.

Limit: an attribute can't name a node (waits for node references).
